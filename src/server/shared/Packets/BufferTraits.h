#pragma once

/************************************************
* BufferTraits.h
* Declares generic, specializable traits for handling
* IO of data for all sorts of Types. 
*
* Usage:
*
* 1) To register a type, specialize 
    template<>
    constexpr bool IsByteBufferType<YOUR_TYPE> = true;

*   Thereafter, it can be used by the buffers, but only via memcpy
*
* 2) You can customize the IO for your type by specializating one or all of the following functions:

    Type read_type(ByteBuffer& buffer)
    void write_type(ByteBuffer& buffer, const Type& value)
        => give you access to serialize the data any way you want

    Type output_type(Remapped&& value)
        => used for translating remapped types back to their source-type (see below)

    void check_type(const Type& value)
        => allows you to insert a customized check, after read-access (to abort the process, throw one of the ByteBuffer-exceptions)

  3) If you want your type to be automatically converted to and from any other known type, specialize:

    template<>
    struct IsByteBufferAcceptedType<YOUR_TYPE>
    {
        static constexpr bool value = true;
    };
    (NOTE: also supports SFINEA for a group of types, via the second optional template-parameter)

    and you are done. Remapped types do not need to be manually registered as in 1)
*
*
*************************************************/

#include "BaseByteBuffer.h"
#include "MethodReflection.h"

DEFINE_METHOD_REFLECTION_SIGNATURE(read_modify, templates<>,)

class ByteBuffer;

namespace packet
{
    // //
    // IO functions - dictates basic value IO, but aallows special handling for types (ie. strings)
    // //

    template<typename Type>
    Type read_type(BaseByteBuffer& buffer)
    {
        return buffer.read_raw<Type>();
    }
    // how the value gets written to the buffer
    template<typename Type>
    void write_type(BaseByteBuffer& buffer, const Type& value)
    {
        buffer.append_raw<Type>(value);
    }
    // converts the internally used value to the specified output type
    template<typename Type, typename Remapped>
    Type output_type(Remapped&& value)
    {
        // Check if we can convert RemappedType back to Type
        // otherwise, user will have to specialize this function
        if constexpr(std::is_same_v<Type, Remapped>)
            return std::move(value);
        else if constexpr (std::is_constructible_v<Type, Remapped&&>)
            return Type(std::move(value));
        else
            return static_cast<Type>(value);
    }

    // bind to struct, to support SFINEA
    template<typename Type, typename = void>
    struct BufferIO
    {
        static Type read(BaseByteBuffer& buffer) { return read_type<Type>(buffer); }
        static void write(BaseByteBuffer& buffer, const Type& value) { return write_type(buffer, value); }
    };

    // types can optionally check if the received values is valid
    template<typename Type>
    void check_type(const Type& value)
    {
    }

    template<typename Buffer, typename Type>
    constexpr bool ReadByModify = Reflect::read_modify_::Class<BufferIO<Type>>::template Arguments<Buffer&, Type&>::template Exists();


    // //
    // Remapping - allows to redirect types, eigther for custom values (ie. enum classes) or optimization (std::string as std::string_view)
    // //

    template<typename Type>
    struct ByteBufferRemapped
    {
        using type = Type;
    };
    template<typename Type>
    using ByteBufferRemappedType = typename ByteBufferRemapped<Type>::type;
    template<typename Type>
    constexpr bool IsByteBufferRemapped = !std::is_same_v<ByteBufferRemappedType<Type>, Type>;

    // maps internal value back to original
    template<typename Type, typename Remapped>
    struct RevertRemapping
    {
        static Type output(Remapped&& value) { return output_type<Type>(std::move(value)); }
    };


    // //
    // Type registration - check if types are known to the buffer
    // //

    // todo: find out why this doesn't work with SFINEA on IsByteBufferRemapped
    // remapped types are automatically accepted => todo: only if remapped type is too
    template<typename Type, typename = void>
    struct IsByteBufferAcceptedType
    {
        static constexpr bool value = IsByteBufferRemapped<Type>;
    };
    template<typename Type>
    constexpr bool IsByteBufferType = IsByteBufferAcceptedType<Type>::value;

    template<typename Type>
    using EnableIfBufferType = std::enable_if_t<IsByteBufferType<Type>>;


    // //
    // Tag - serialize type in another format (ie. packed values, container with different size-type, ...)
    // //

    template<typename Tag, typename Type>
    struct BufferTag final
    {
        explicit BufferTag(Type& value) : value(value) {}
        explicit BufferTag(const Type& value) : value(const_cast<Type&>(value)) {}

        void read(ByteBuffer& buffer) { Tag::read(value, buffer); }
        void write(ByteBuffer& buffer) { Tag::write(value, buffer); }

    private:
        Type & value;
    };

    template<typename Tag, typename Type>
    struct IsByteBufferAcceptedType<BufferTag<Tag, Type>> : std::conditional_t<IsByteBufferType<Type>, std::true_type, std::false_type> {};

    template<typename Tag, typename Type>
    struct BufferIO<BufferTag<Tag, Type>>
    {
        static void read_modify(ByteBuffer& buffer, BufferTag<Tag, Type> value)
        {
            value.read(buffer);
        }

        static void write(ByteBuffer& buffer, BufferTag<Tag, Type> value)
        {
            value.write(buffer);
        }
    };

    // create function
    template<typename Tag, typename Type>
    auto with_tag(Type& value) { return BufferTag<Tag, Type>(value); }
    template<typename Tag, typename Type>
    auto with_tag(const Type& value) { return BufferTag<Tag, Type>(value); }

}


// //
// Basic types - declare some standard-types like int/floats
// //

// all int/float types
template<typename Type>
struct packet::IsByteBufferAcceptedType<Type, std::enable_if_t<std::is_arithmetic_v<Type>>> : std::true_type {};
// all enums => todo:check/ensure correct size?
template<typename Type>
struct packet::IsByteBufferAcceptedType<Type, std::enable_if_t<std::is_enum_v<Type>>> : std::true_type {};

// bool gets remapped to uint8, for safety
template<>
struct packet::ByteBufferRemapped<bool> { using type = uint8; };