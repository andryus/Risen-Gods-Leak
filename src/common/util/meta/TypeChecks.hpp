#pragma once

namespace util::meta
{
    namespace impl
    {
        struct ReportType final {};
    }

    template<typename T>
    constexpr bool False = false;

    template<typename T>
    constexpr bool ReportExactType()
    {
        // invalid conversion will force the compiler to show the precise type
        const T x = impl::ReportType();

        // constexpr-usage ensures this is called before rest of function
        return true;
    }

    template<typename T>
    void Unreferenced(const T&) { }


}
