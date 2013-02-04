#ifndef SOFTDSP_AT_HPP
#define SOFTDSP_AT_HPP

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
#include <softdsp/plus.hpp>
#include <softdsp/minus.hpp>
#include <softdsp/context_definitions.hpp>
#include <softdsp/mpl.hpp>

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
#include <iostream>
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
  namespace proto = boost::proto;
  using proto::argsns_::list2;
  using proto::exprns_::expr;

  namespace keywords {
    template< typename Index >
    struct at {
      template< typename Context >
      class eval {
      public:
        eval( const Context &context_ ) : tools( context_.get_toolbox() ) {}
        template< typename ValueType >
        return_value<
          typename boost::mpl::at<
            typename boost::remove_reference<
              typename get_return_type< ValueType >::type
            >::type,
            Index
          >::type
        >
        operator()(
          ValueType value_,
          typename boost::enable_if<
            boost::mpl::and_<
              at_least_one_operand_is_llvm_value< ValueType >,
              boost::fusion::traits::is_sequence<
                typename boost::remove_reference<
                  typename get_return_type< ValueType >::type
                >::type
              >,
              boost::mpl::not_<
                boost::is_reference<
                  typename get_return_type< ValueType >::type
                >
              >
            >
          >::type* = 0
        ) {
          const auto value = tools->as_llvm_value( tools->load( value_ ) );
          std::vector< unsigned int > args = { static_cast< unsigned int >( Index::value ) };
          llvm::ArrayRef< unsigned int > args_ref( args );
          std::cout << "debug1: " << Index::value << std::endl;
          return return_value<
            typename boost::mpl::at<
              typename boost::remove_reference<
                typename get_return_type< ValueType >::type
              >::type,
              Index
            >::type
          >(
            tools->ir_builder.CreateExtractValue( value.value, args_ref )
          );
        }
        template< typename ValueType >
        return_value<
          typename boost::mpl::at<
            typename boost::remove_reference<
              typename get_return_type< ValueType >::type
            >::type,
            Index
          >::type&
        >
        operator()(
          ValueType value_,
          typename boost::enable_if<
            boost::mpl::and_<
              at_least_one_operand_is_llvm_value< ValueType >,
              boost::fusion::traits::is_sequence<
                typename boost::remove_reference<
                  typename get_return_type< ValueType >::type
                >::type
              >,
              boost::is_reference<
                typename get_return_type< ValueType >::type
              >
            >
          >::type* = 0
        ) {
          const auto value = tools->as_llvm_value( value_ );
          std::cout << "debug2: " << Index::value << std::endl;
          return return_value<
            typename boost::mpl::at<
              typename boost::remove_reference<
                typename get_return_type< ValueType >::type
              >::type,
              Index
            >::type&
          >(
            tools->ir_builder.CreateStructGEP( value.value, Index::value )
          );
        }
        template< typename ValueType >
        auto operator()(
          ValueType value_,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::mpl::not_<
                at_least_one_operand_is_llvm_value< ValueType >
              >,
              boost::fusion::traits::is_sequence<
                ValueType
              >
            >
          >::type* = 0
        ) -> typename softdsp::mpl::at< ValueType, Index >::type {
          return boost::fusion::at< Index >( value_ );
        }
      private:
        const typename Context::toolbox_type tools;
      };
    };
  }
  
  template< typename Index, typename Expr >
  auto at( Expr expr )
  -> decltype( std::declval< typename boost::proto::terminal< keywords::at< Index > >::type >()( expr ) ) {
    typename boost::proto::terminal< keywords::at< Index > >::type at_;
    return at_( expr );
  }
  template< size_t index, typename Expr >
  auto at_c( Expr expr )
  -> decltype( std::declval< typename boost::proto::terminal< keywords::at< boost::mpl::size_t< index > > >::type >()( expr ) ) {
    typename boost::proto::terminal< keywords::at< boost::mpl::size_t< index > > >::type at_;
    return at_( expr );
  }
}

#endif
