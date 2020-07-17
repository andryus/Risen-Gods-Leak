#pragma once
#include <boost/test/data/test_case.hpp>
#include "Define.h"

namespace test
{

    namespace bdata = boost::unit_test::data;

    template<class Container, typename Remapper>
    class ContainerDataset {
    public:
        using ContainerItr = typename Container::const_iterator;

        using sample = decltype(std::declval<Remapper>()(*std::declval<ContainerItr>()));

        enum { arity = 1 };

        struct iterator {

            iterator(ContainerItr tableItr, const Remapper& remapper) : itr(tableItr), remapper(remapper) {}

            sample operator*() const { return remapper(*itr); }
            void operator++()
            {
                ++itr;
            }
        private:
            ContainerItr itr;
            const Remapper& remapper;
        };

        ContainerDataset(const Container& container, Remapper remapper) : container(container), remapper(remapper) {}

        bdata::size_t size() const { return std::size(container); }

        // iterator
        iterator begin() const { return iterator(std::begin(container), remapper); }

        Container container;
        Remapper remapper;
    };

    template<class Container, typename Remapper>
    auto createContainerDataset(const Container& container, Remapper remapper)
    {
        return ContainerDataset<Container, Remapper>(container, remapper);
    }

    template<class Container>
    auto createContainerDataset(const Container& container)
    {
        constexpr auto _defaultRemapper = [](const auto& value) { return value; };

        return createContainerDataset(std::move(container), _defaultRemapper);
    }

}

namespace boost::unit_test::data::monomorphic {
    // registering fibonacci_dataset as a proper dataset
    template<class Container, class Remapper>
    struct is_dataset<test::ContainerDataset<Container, Remapper>> : boost::mpl::true_ {};
}