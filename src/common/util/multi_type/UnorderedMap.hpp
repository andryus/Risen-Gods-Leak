#pragma once

#include <tuple>
#include <unordered_map>

#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

#include "util/Typelist.hpp"

namespace util::multi_type
{

    namespace detail
    {
        template<typename KeyT, typename ...ValueTs>
        class UnorderedMapImpl
        {
        public:
            template<typename ValueT>
            bool IsEmpty() const
            {
                return get<ValueT>().empty();
            }

            template<typename ValueT>
            size_t Size() const
            {
                return get<ValueT>().size();
            }

            template<typename ValueT>
            boost::optional<ValueT&> Find(KeyT const& key)
            {
                auto& container = get<ValueT>();
                auto itr = container.find(key);

                return itr != container.end() ? boost::optional<ValueT&>{ itr->second } : boost::none;
            }

            template<typename ValueT>
            boost::optional<ValueT const&> Find(KeyT const& key) const
            {
                auto const& container = get<ValueT>();
                auto itr = container.find(key);

                return itr != container.end() ? boost::optional<ValueT const&>{ itr->second } : boost::none;
            }

            template<typename ValueT>
            auto GetValues()
            {
                return get<ValueT>() | boost::adaptors::map_values;
            }

            template<typename ValueT>
            auto GetValues() const
            {
                return get<ValueT>() | boost::adaptors::map_values;
            }

            template<typename ValueT>
            std::pair<ValueT&, bool> Insert(KeyT const& key, ValueT value)
            {
                auto result = get<ValueT>().emplace(key, std::move(value));
                return std::pair<ValueT&, bool>(result.first->second, result.second);
            }

            template<typename ValueT>
            bool Remove(KeyT const& key)
            {
                return get<ValueT>().erase(key) != 0;
            }

            template<typename ValueT>
            void Clear()
            {
                get<ValueT>().clear();
            }

        private:
            template<typename ContainerKeyT, typename ContainerValueT>
            using ContainerType = std::unordered_map<ContainerKeyT, ContainerValueT>;

            template<typename T>
            ContainerType<KeyT, T>& get()
            {
                return std::get<ContainerType<KeyT, T>>(tuple);
            }

            std::tuple<ContainerType<KeyT, ValueTs>...> tuple{};
        };
    }

    template<typename KeyT, typename ...ValueTs>
    using UnorderedMap = UnpackVariadicTypelist<
        detail::UnorderedMapImpl,
        Fixed<KeyT>,
        Variadic<ValueTs...>
    >;

}
