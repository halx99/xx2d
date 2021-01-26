﻿#include <type_traits>
#include <vector>
#include <tuple>
#include <array>
#include <string>
#include <cassert>

#include <iostream>

#include "LinkPool.h"


// 模拟一下 ECS 的内存分布特性（ 按业务需求分组的 数据块 连续高密存放 )
// 组合优先于继承, 拆分后最好只有数据, 没有函数

/*********************************************************************/
// 基础库代码

// 每个 非 pod 切片, 需包含这个宏, 确保 move 操作的高效（ 省打字 )

#define SliceBaseCode(T)			\
T() = default;						\
T(T const&) = delete;				\
T& operator=(T const&) = delete;	\
T(T&&) = default;					\
T& operator=(T&&) = default;


// 组合后的类的基础数据 int 包装, 便于 tuple 类型查找定位

template<typename T>
struct I {
	int i;
	operator int& () { return i; }
	operator int const& () const { return i; }
};


// 组合后的类之本体. 使用 using 来定义类型

template<typename...TS>
struct O {
	O() = default;
	O(O const&) = delete;
	O& operator=(O const&) = delete;
	O(O&&) = default;
	O& operator=(O&&) = default;

	int next, prev;					// next 布置在这里，让 LinkPool 入池后覆盖使用
	uint32_t refCount, version;		// Shared 只关心 refCount. Weak 可能脏读 version ( 确保不被 LinkPool 覆盖 )
	std::tuple<I<TS>...> data;
};


// 判断 tuple 里是否存在某种数据类型

template <typename T, typename Tuple>
struct HasType;

template <typename T>
struct HasType<T, std::tuple<>> : std::false_type {};

template <typename T, typename U, typename... Ts>
struct HasType<T, std::tuple<U, Ts...>> : HasType<T, std::tuple<Ts...>> {};

template <typename T, typename... Ts>
struct HasType<T, std::tuple<T, Ts...>> : std::true_type {};

template <typename T, typename Tuple>
using TupleContainsType = typename HasType<T, Tuple>::type;

template <typename T, typename Tuple>
constexpr bool TupleContainsType_v = TupleContainsType<T, Tuple>::value;


// 计算某类型在 tuple 里是第几个

template <class T, class Tuple>
struct TupleTypeIndex;

template <class T, class...TS>
struct TupleTypeIndex<T, std::tuple<T, TS...>> {
	static const size_t value = 0;
};

template <class T, class U, class... TS>
struct TupleTypeIndex<T, std::tuple<U, TS...>> {
	static const size_t value = 1 + TupleTypeIndex<T, std::tuple<TS...>>::value;
};

template <typename T, typename Tuple>
constexpr size_t TupleTypeIndex_v = TupleTypeIndex<T, Tuple>::value;


// 切片容器 tuple 套 vector

template<typename...TS>
struct Vectors {
	using Types = std::tuple<TS...>;
	std::tuple<std::vector<TS>...> vectors;
	std::array<std::vector<std::pair<int, int>>, sizeof...(TS)> ownerss;	// first: typeId( Items 的 Types 的第几个 )     second: index

	template<typename T> std::vector<T> const& Get() const { return std::get<std::vector<T>>(vectors); }
	template<typename T> std::vector<T>& Get() { return std::get<std::vector<T>>(vectors); }
	template<typename T> std::vector<std::pair<int, int>> const& GetIdxs() const { return ownerss[TupleTypeIndex_v<T, Types>]; }
	template<typename T> std::vector<std::pair<int, int>>& GetIdxs() { return ownerss[TupleTypeIndex_v<T, Types>]; }
};

// Item容器 tuple 套 LinkPool ( 确保分配出来的 idx 不变 )

template<typename...TS>
struct LinkPools {
	using Types = std::tuple<TS...>;
	std::tuple<LinkPool<TS>...> linkpools;
	template<typename T> LinkPool<T> const& Get() const { return std::get<LinkPool<T>>(linkpools); }
	template<typename T> LinkPool<T>& Get() { return std::get<LinkPool<T>>(linkpools); }
};



template<typename Slices, typename Items>
struct Env {
	/*********************************************************************/
	// 拆分后的数据块 容器
	Slices slices;

	template<typename T>
	void CreateData(int const& typeId, int const& owner, int& idx) {
		auto& s = slices.Get<T>();
		auto& n = slices.GetIdxs<T>();
		idx = (int)s.size();
		s.emplace_back();
		n.emplace_back(typeId, owner);
		assert(s.size() == n.size());
	}
	template<typename T>
	void ReleaseData(int const& idx) {
		auto& s = slices.Get<T>();
		auto& n = slices.GetIdxs<T>();
		assert(idx >= 0 && idx < s.size());
		s[idx] = std::move(s.back());
		n[idx] = n.back();
		s.pop_back();
		n.pop_back();
		assert(s.size() == n.size());
	}

	template<size_t i = 0, typename Tp, typename T>	std::enable_if_t<i == std::tuple_size_v<Tp>> TryCreate(int const& typeId, int const& owner, T& r) {}
	template<size_t i = 0, typename Tp, typename T>	std::enable_if_t < i < std::tuple_size_v<Tp>> TryCreate(int const& typeId, int const& owner, T& r) {
		using O = std::decay_t<decltype(std::get<i>(std::declval<Tp>()))>;
		if constexpr (TupleContainsType_v<I<O>, T>) {
			CreateData<O>(typeId, owner, std::get<I<O>>(r));
		}
		TryCreate<i + 1, Tp, T>(typeId, owner, r);
	}

	template<size_t i = 0, typename Tp, typename T>	std::enable_if_t<i == std::tuple_size_v<Tp>> TryRelease(T const& r) {}
	template<size_t i = 0, typename Tp, typename T>	std::enable_if_t < i < std::tuple_size_v<Tp>> TryRelease(T const& r) {
		using O = std::decay_t<decltype(std::get<i>(std::declval<Tp>()))>;
		if constexpr (TupleContainsType_v<I<O>, T>) {
			ReleaseData<O>(std::get<I<O>>(r));
		}
		TryRelease<i + 1, Tp, T>(r);
	}

	/*********************************************************************/
	// Items 容器
	Items items;
	// 用于版本号自增填充
	uint32_t versionGen = 0;
	// 针对每种类型的 链表头
	std::array<int, std::tuple_size_v<typename Items::Types>> headers;

	template<typename T>
	int& Header() {
		return headers[TupleTypeIndex_v<T, typename Items::Types>];
	}

	Env() {
		headers.fill(-1);
	}

	template<typename T>
	struct Shared {
		using ElementType = T;
		Env* env;
		int idx;

		~Shared() { Reset(); }
		Shared() : env(nullptr), idx(-1) {}
		Shared(Env* const& env, int const& idx) : env(env), idx(idx) {}
		Shared(Shared&& o) noexcept { env = o.env; idx = o.idx; o.env = nullptr; o.idx = -1; }
		Shared(Shared const& o) : env(o.env), idx(o.idx) { if (o.env) { ++o.env->RefItem<T>(o.idx).refCount; } }

		Shared& operator=(Shared const& o) { Reset(o.env, o.idx); return *this; }
		Shared& operator=(Shared&& o) { std::swap(env, o.env); std::swap(idx, o.idx); return *this; }

		bool operator==(Shared const& o) const noexcept { return env == o.env && idx == o.idx; }
		bool operator!=(Shared const& o) const noexcept { return env != o.env || idx != o.idx; }

		//struct Weak<T> ToWeak() const noexcept;

		T& Ref() {
			assert(env);
			return env->RefItem<T>(idx);
		}
		template<typename Slice>
		Slice& RefSlice() {
			assert(env);
			return env->RefSlice<Slice>(Ref());
		}

		explicit operator bool() const noexcept { return env != nullptr; }
		bool Empty() const noexcept { return env == nullptr; }
		uint32_t refCount() const noexcept { if (!env) return 0; return env->RefItem(idx).refCount; }

		void Reset() {
			if (!env) return;
			auto& o = Ref();
			assert(o.refCount);
			if (--o.refCount == 0) {
				env->ReleaseItem<T>(idx);
			}
			env = nullptr;
		}

		void Reset(Env* const& env, int const& idx) {
			if (this->env == env && this->idx = idx) return;
			Reset();
			if (env) {
				this->env = env;
				this->idx = idx;
				auto& o = env->RefItem<T>(idx);
				this->version = o.version;
				++o.refCount;
			}
		}
	};

	// todo: Weak

	template<typename T>
	Shared<T> CreateItem() {
		auto& s = items.Get<T>();
		auto& h = Header<T>();
		int idx;
		s.Alloc(idx);
		auto& r = s[idx];
		TryCreate<0, typename decltype(slices)::Types>(TupleTypeIndex_v<T, typename Items::Types>, idx, r.data);
		r.version = ++versionGen;
		r.refCount = 1;
		r.prev = -1;
		if (h != -1) {
			s[h].prev = idx;
		}
		r.next = h;
		h = idx;
		return Shared<T>(this, idx);
	}

	template<typename T>
	void ReleaseItem(int const& idx) {
		auto& s = items.Get<T>();
		auto& h = Header<T>();
		auto& r = s[idx];
		assert(r.refCount == 0);
		r.version = 0;
		TryRelease<0, typename decltype(slices)::Types>(r.data);
		if (h == idx) {
			h = r.next;
		}
		else/* if (r.prev != -1)*/ {
			s[r.prev].next = r.next;
		}
		if (r.next != -1) {
			s[r.next].prev = r.prev;
		}
		s.Free(idx);
	}

	/*********************************************************************/

	template<typename Item>
	Item& RefItem(int const& idx) {
		return items.Get<Item>()[idx];
	}
	template<typename Item>
	Item const& RefItem(int const& idx) const {
		return items.Get<Item>()[idx];
	}

	template<typename Slice, typename Item>
	Slice& RefSlice(Item& o) {
		auto&& idx = std::get<I<Slice>>(o.data);
		return slices.Get<Slice>()[idx];
	}
	template<typename Slice, typename Item>
	Slice const& RefSlice(Item const& o) const {
		auto&& idx = std::get<I<Slice>>(o.data);
		return slices.Get<Slice>()[idx];
	}

	template<typename Item, typename F>
	void ForeachItems(F&& f) {
		int h = Header<Item>();
		while (h != -1) {
			auto& o = RefItem<Item>(h);
			h = o.next;
			f(o);
		}
	}

	template<typename Slice, typename F>
	void ForeachSlices(F&& f) {
		auto& s = slices.Get<Slice>();
		auto& n = slices.GetIdxs<Slice>();
		auto siz = s.size();
		for (size_t i = 0; i < siz; ++i) {
			f(s[i], n[i]);
		}
	}
};

/*********************************************************************/
// tests

// slices
struct A {
	SliceBaseCode(A);
	std::string name;
};
struct B {
	float x, y;
};
struct C {
	int hp;
};

// items
using ABC = O<A, B, C>;
using AB = O<A, B>;
using AC = O<A, C>;
using BC = O<B, C>;

using MyEnv = Env<Vectors<A, B, C>, LinkPools<ABC, AB, AC, BC>>;

std::array<char const*, 4> itemTypes { "ABC", "AB", "AC", "BC" };

int main() {
	MyEnv env;

	auto&& abc = env.CreateItem<ABC>();
	abc.RefSlice<A>().name = "asdf";
	abc.RefSlice<B>().x = 1;
	abc.RefSlice<B>().y = 2;
	abc.RefSlice<C>().hp = 3;

	// 出括号后自杀
	{
		auto&& bc = env.CreateItem<BC>();
		bc.RefSlice<B>().x = 1.2f;
		bc.RefSlice<B>().y = 3.4f;
		bc.RefSlice<C>().hp = 5;
	}

	auto&& ac = env.CreateItem<AC>();
	ac.RefSlice<A>().name = "qwerrrr";
	ac.RefSlice<C>().hp = 7;

	auto&& ac2 = env.CreateItem<AC>();
	ac2.RefSlice<A>().name = "e";
	ac2.RefSlice<C>().hp = 9;

	env.ForeachSlices<A>([](A& o, auto& owner) {
		std::cout << "owner: " << itemTypes[owner.first] << ", idx = " << owner.second << std::endl;
		std::cout << "A::name = " << o.name << std::endl;
		});
	env.ForeachSlices<B>([](B& o, auto& owner) {
		std::cout << "owner: " << itemTypes[owner.first] << ", idx = " << owner.second << std::endl;
		std::cout << "B::x y = " << o.x << ", " << o.y << std::endl;
		});
	env.ForeachSlices<C>([](C& o, auto& owner) {
		std::cout << "owner: " << itemTypes[owner.first] << ", idx = " << owner.second << std::endl;
		std::cout << "C::hp = " << o.hp << std::endl;
		});

	return 0;
}
























//struct Bar {
//	Bar() = default;
//	Bar(Bar const&) {
//		std::cout << "Bar const&" << std::endl;
//	}
//	Bar& operator=(Bar const&) {
//		std::cout << "=Bar const&" << std::endl;
//		return *this;
//	}
//	Bar(Bar&&) { 
//		std::cout << "Bar&&" << std::endl;
//	}
//	Bar& operator=(Bar&&) { 
//		std::cout << "=Bar&&" << std::endl;
//		return *this;
//	}
//};
//struct Foo { 
//	SliceBaseCode(Foo)
//	Bar bar;
//};
