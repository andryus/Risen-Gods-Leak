#pragma once

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "util/predicate/Predicates.hpp"
#include "util/transform/Transforms.hpp"

namespace util::adaptor
{

    namespace impl
    {

        template<class LeftHolder, class RightHolder>
        struct combine_holder
        {
            LeftHolder left;
            RightHolder right;
            combine_holder(LeftHolder left, RightHolder right) :
                left(left),
                right(right)
            {}
        };


        template<class SinglePassRange, class LeftHolder, class RightHolder>
        inline auto operator|(SinglePassRange& range, const combine_holder<LeftHolder, RightHolder>& holder)
        {
            return (range | holder.left) | holder.right;
        }

        template<class SinglePassRange, class LeftHolder, class RightHolder>
        inline auto operator|(const SinglePassRange& range, const combine_holder<LeftHolder, RightHolder>& holder)
        {
            return (range | holder.left) | holder.right;
        }
        
    }

    template<typename LeftHolder, typename RightHolder>
    inline auto operator+(const LeftHolder& left, const RightHolder& right)
    {
        return impl::combine_holder<LeftHolder, RightHolder>(left, right);
    }


    const auto UnwrapReference = boost::adaptors::transformed(transform::UnwrapReference);


    const auto FilterOptional = boost::adaptors::filtered(predicate::HasValue);

    const auto UnwrapOptional = boost::adaptors::transformed(transform::UnwrapOptional);

    const auto FilterAndUnwrapOptional = FilterOptional + UnwrapOptional;

}
