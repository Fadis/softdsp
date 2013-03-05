#ifndef SOFTDSP_CONTEXT_DEFINITIONS_HPP
#define SOFTDSP_CONTEXT_DEFINITIONS_HPP

#include <softdsp/primitive.hpp>
#include <softdsp/constant_generator.hpp>
#include <softdsp/tag.hpp>
#include <softdsp/type_generator.hpp>
#include <softdsp/nth_type.hpp>
#include <softdsp/data_layout/load_value_at.hpp>
#include <softdsp/data_layout/store_value_at.hpp>
#include <softdsp/data_layout/common.hpp>
#include <softdsp/data_layout/proxy.hpp>
#include <softdsp/data_layout/get_element_type.hpp>
#include <softdsp/data_layout/array.hpp>
#include <softdsp/data_layout/tuple.hpp>
#include <softdsp/placeholders.hpp>
#include <softdsp/llvm_toolbox.hpp>
#include <softdsp/return_value.hpp>
#include <softdsp/dereference.hpp>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/BasicBlock.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/DataLayout.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/TargetRegistry.h>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/range/reference.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/boost_array.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/mpl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/preprocessor/enum.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/repeat.hpp>
#include <boost/swap.hpp>
#include <boost/proto/proto.hpp>
#include <boost/container/flat_map.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>

namespace softdsp {
  template< typename ...args >
  struct at_least_one_operand_is_llvm_value : public boost::mpl::bool_< false > {};

  template< typename head, typename ...tail >
  struct at_least_one_operand_is_llvm_value< head, tail... >
    : boost::mpl::or_<
      boost::is_convertible< head, return_value_ >,
      at_least_one_operand_is_llvm_value< tail... >
    > {};

  template< typename ...args >
  struct all_operands_are_llvm_value : public boost::mpl::bool_< true > {};

  template< typename head, typename ...tail >
  struct all_operands_are_llvm_value< head, tail... >
    : boost::mpl::and_<
      boost::is_convertible< head, return_value_ >,
      all_operands_are_llvm_value< tail... >
    > {};

  template< typename ...args >
  struct at_least_one_operand_is_primitive : public boost::mpl::bool_< false > {};

  template< typename head, typename ...tail >
  struct at_least_one_operand_is_primitive< head, tail... >
    : public boost::mpl::or_<
      is_primitive< head >,
      at_least_one_operand_is_primitive< tail... >
    > {};

  template< typename ...args >
  struct all_operands_are_primitive : public boost::mpl::bool_< true > {};

  template< typename head, typename ...tail >
  struct all_operands_are_primitive< head, tail... >
    : boost::mpl::and_<
      is_primitive< head >,
      all_operands_are_primitive< tail... >
    > {};

  template<
    typename T,
    bool is_range = hermit::is_forward_traversal_range< T >::value
  >
  struct is_float : public boost::is_float< T > {};
  
  template< typename T >
  struct is_float< T, true > : public boost::is_float< typename boost::range_value< T >::type > {};

  template< typename ...args >
  struct at_least_one_operand_is_float : public boost::mpl::bool_< false > {};

  template< typename head, typename ...tail >
  struct at_least_one_operand_is_float< head, tail... >
    : public boost::mpl::or_<
      is_float< head >,
      at_least_one_operand_is_float< tail... >
    > {};
}

#define ONE_OR_MORE_UNARY_OPERANDS_ARE_LLVM_VALUE \
  boost::mpl::or_< \
    boost::is_convertible< ValueType, return_value_ > \
  >

#define ONE_OR_MORE_BINARY_OPERANDS_ARE_LLVM_VALUE \
  boost::mpl::or_< \
    boost::is_convertible< LeftType, return_value_ >, \
    boost::is_convertible< RightType, return_value_ > \
  >

#define ALL_OF_BINARY_OPERANDS_ARE_PRIMITIVE \
  boost::mpl::or_< \
    is_primitive<  >, \
    boost::is_convertible< RightType, return_value_ > \
  >


#define SOFTDSP_ENABLE_BINARY_OPERATOR( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_, proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< context_type& >(). name ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ), \
          proto::eval( boost::proto::right( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        const auto left = proto::eval( boost::proto::left( expr ), context ); \
        const auto right = proto::eval( boost::proto::right( expr ), context ); \
        return context. name ( left, right ); \
      } \
    };

#define SOFTDSP_ENABLE_UNARY_OPERATOR( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< context_type >(). name ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        return context. name ( \
            proto::eval( boost::proto::left( expr ), context ) \
          ); \
      } \
    };

#define SOFTDSP_ENABLE_BINARY_OPERATOR_( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_, proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< keywords :: name < context_type > >() ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ), \
          proto::eval( boost::proto::right( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        keywords :: name < context_type > function( context ); \
        return function ( \
            proto::eval( boost::proto::left( expr ), context ), \
            proto::eval( boost::proto::right( expr ), context ) \
          ); \
      } \
    };

#define SOFTDSP_ENABLE_UNARY_OPERATOR_( name ) \
    template< typename Expr > struct eval< \
      Expr, \
      typename boost::enable_if< \
        proto::matches< Expr, proto:: name < proto::_ > > \
      >::type \
    > { \
      typedef decltype( \
        std::declval< keywords :: name < context_type > >() ( \
          proto::eval( boost::proto::left( std::declval< Expr& >() ), std::declval< context_type& >() ) \
        ) \
      ) result_type; \
      result_type operator()( Expr &expr, context_type &context ) { \
        keywords :: name < context_type > function( context ); \
        return function ( \
            proto::eval( boost::proto::left( expr ), context ) \
          ); \
      } \
    };

#endif
