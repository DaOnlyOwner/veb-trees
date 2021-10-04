#pragma once
#include "definitions.h"
#include <optional>
#include <vector>
#include <memory>
#include <array>
#include <utility>
#include <limits>
#include <type_traits>

namespace doo
{

	namespace detail
	{

		constexpr inline u64 pow_2(uint in) { return 1ULL << in; }
		constexpr inline bool is_null(u64 in) { return in == std::numeric_limits<u64>::max(); }
		constexpr inline bool is_not_null(u64 in) { return in != std::numeric_limits<u64>::max(); }

		template<uint log2_u, bool shrink_to_fit, typename Enable = void>
		class veb
		{
		public:
			static_assert(log2_u > 0 && log2_u <= 63, "log2_u must be within [0,63]");
			bool member(u64 x)
			{
				if (x == min || x == max) return true; 
				if (x < min || x > max) return false; // Not within range
				u64 h = high(x);
				u64 l = low(x);
				make_subcluster(h);
				return cluster[h]->member(l); // Tail call recursion
			}

			bool empty() const
			{
				return is_null(min);
			}

			void renew_key(u64 x_old, u64 x_new)
			{
				del(x_old);
				del(x_new);
			}

			void insert(u64 x)
			{
				if (empty())
				{
					init(x);
					return;
				}

				if (x < min)
					std::swap(x, min);
				if (x > max)
					max = x;

				u64 h = high(x);
				u64 l = low(x);
				make_subcluster(h);

				if (cluster[h]->empty())
				{
					make_summary();
					summary->insert(h);
					cluster[h]->init(l);
				}
				else cluster[h]->insert(l);
			}

			u64 succ(u64 x) 
			{
				if (is_not_null(min) && x < min) return min;
				u64 h = high(x);
				u64 l = low(x);
				make_subcluster(h);
				auto c_max = cluster[h]->max;
				if (is_not_null(c_max) && l < c_max)
				{
					u64 j = cluster[h]->succ(l);
					return idx(h, j);
				}

				make_summary();
				auto h_succ = summary->succ(h);
				if (is_null(h_succ)) return nullop;
				u64 j = cluster[h_succ]->min;
				return idx(h_succ, j);
			}

			u64 pred(u64 x)
			{
				if (is_not_null(max) && x > max) return max;

				u64 h = high(x);
				u64 l = low(x);
				make_subcluster(h);
				auto c_min = cluster[h]->min;
				if (is_not_null(c_min) && l > c_min)
				{
					u64 j = cluster[h]->pred(l);
					return idx(h, j);
				}

				make_summary();
				auto h_pred = summary->pred(h);
				if (is_null(h_pred))
				{
					if (is_not_null(min) && x > min) return min;
					return nullop;
				}
				u64 j = cluster[h_pred]->max;
				return idx(h_pred, j);
			}

			void del(u64 x)
			{
				if (min == max)
				{
					min = nullop;
					max = nullop;
					if constexpr (shrink_to_fit) summary = nullptr; // dealloc
					return;
				}

				if (x == min)
				{
					u64 i = summary->min;
					x = idx(i, cluster[i]->min);
					min = x;
				}
				u64 h = high(x);
				u64 l = low(x);
				cluster[h]->del(l);
				if (cluster[h]->empty())
				{
					if constexpr (shrink_to_fit) cluster[h] = nullptr;
					summary->del(h);
					if (x == max)
					{
						auto s_max = summary->max;
						if (is_null(s_max)) max = min;
						else max = idx(s_max, cluster[s_max]->max);

					}
				}
				else if (x == max) max = idx(h, cluster[h]->max);
			}
			
			void init(u64 x)
			{
				min = x;
				max = x;
			}


			u64 min = nullop, max = nullop;
		private:
			constexpr static u64 nullop = std::numeric_limits<u64>::max();
			//constexpr key u = pow_2(log2_u);
			constexpr static u64 sqrt_u_floor = pow_2(log2_u / 2);
			constexpr static u64 sqrt_u_ceil =  pow_2((log2_u + 1) / 2);
			// Eg: 0001 << (log2_u / 2)(=2) -> 0100 - 0001 = 0011 
			constexpr static u64 lower_mask = sqrt_u_floor /*Set lower half bit*/ - 1;
			constexpr static int half_log2_u = log2_u / 2;
			constexpr static int half_log2_u_ceil = (log2_u + 1) / 2;
			std::unique_ptr<veb<half_log2_u_ceil,shrink_to_fit>> summary;
			std::array<std::unique_ptr<veb<half_log2_u,shrink_to_fit>>, sqrt_u_ceil> cluster;

			constexpr u64 high(u64 in) { return in >> half_log2_u; } // eg: 1101. u=4, halfU = 2. 1101 >> 2 == 0011 
			constexpr u64 low(u64 in) { return in & lower_mask; }
			constexpr u64 idx(u64 i, u64 j) { return (i<<half_log2_u) | j; }

			void make_subcluster(u64 h)
			{
				if (cluster[h] == nullptr)
					cluster[h] = std::make_unique<veb<half_log2_u,shrink_to_fit>>();
			}

			void make_summary()
			{
				if (summary == nullptr)
					summary = std::make_unique<veb<half_log2_u_ceil,shrink_to_fit>>();
			}
		};

		template<uint log2_u, bool shrink_to_fit>
		class veb<log2_u, shrink_to_fit, typename std::enable_if<log2_u <= 6>::type>
		{
		public:
			void insert(u64 x)
			{
				bitvector |= pow_2(x);
				if (x < min || is_null(min)) min = x;
				if (x > max || is_null(max)) max = x;
			}

			void del(u64 x)
			{
				if (x == min) min = succ(min);
				if (x == max) max = pred(max);
				bitvector &= ~pow_2(x);
			}

			u64 succ(u64 x)
			{
				u64 shifted = bitvector >> (x + 1);
				if (shifted == 0ULL)
					return nullop;
				return idx_lsb(shifted) + x + 1;
			}

			u64 pred(u64 x)
			{
				// Cannot left shift due to arithmetic overflow which is undefined behavior.
				
				if (x == 0) return nullop;
				/*
				E.g. 01010. pred(3) == 1 -> 00001 << (4 - 3 + 1)) = 00100 - 1 = 00011 << 3 (the argument) == 11000 
				-> ~11000 == 00111 == mask  
				*/
				u64 mask = ~(((1ULL << (63 - x + 1)) - 1) << x);
				uint64_t masked = bitvector & mask;
				if (masked == 0ULL)
					return nullop;
				return idx_msb(masked);
			}

			bool empty() const
			{
				return bitvector == 0;
			}

			bool member(u64 key)
			{
				return !!(bitvector & pow_2(key));
			}

			void init(u64 x)
			{
				bitvector |= pow_2(x);
				min = x;
				max = x;
			}

			u64 min = nullop, max = nullop;
		private:
			constexpr static u64 nullop = std::numeric_limits<u64>::max();


			uint64_t bitvector = 0ULL;

			// Get the index of the least significant set bit.
#ifdef __GNUC__
			unsigned int idx_lsb(u64 k)
			{
				return __builtin_ctzll(k);
			}

			unsigned int idx_msb(u64 k)
			{
				return 63-__builtin_clzll(k);
			}

#elif _MSC_VER
			unsigned int idx_lsb(u64 k)
			{
				unsigned long idx;
				_BitScanForward64(&idx, k);
				return idx;
			}

			unsigned int idx_msb(u64 k)
			{
				unsigned long idx;
				_BitScanReverse64(&idx, k);
				return idx;
			}
#endif


		};

	}

	template<uint log2_u, bool shrink_to_fit = true>
	class veb
	{
	public:
		std::optional<u64> get_min()
		{
			if (detail::is_null(container.min)) return {};
			return container.min;
		}

		std::optional<u64> get_max()
		{
			if (detail::is_null(container.max)) return {};
			return container.max;
		}

		void insert(u64 x)
		{
			container.insert(x);
		}

		void del(u64 x)
		{
			container.del(x);
		}

		std::optional<u64> succ(u64 x)
		{
			u64 s = container.succ(x);
			if (detail::is_null(s)) return {};
			return s;
		}

		std::optional<u64> pred(u64 x)
		{
			u64 s = container.pred(x);
			if (detail::is_null(s)) return {};
			return s;
		}

		void renew_key(u64 x_old, u64 x_new)
		{
			container.renew_key(x_old, x_new);
		}

		std::optional<u64> remove_min()
		{
			u64 min = container.min;
			if (detail::is_null(min)) return {};
			container.del(min);
			return min;
		}

		bool empty() const
		{
			return container.empty();
		}

		bool member(u64 x)
		{
			return container.member(x);
		}


	private:
		detail::veb<log2_u, shrink_to_fit> container;
	};

	// TODO
	template<uint log2_u>
	class veb_prio
	{

	};


}