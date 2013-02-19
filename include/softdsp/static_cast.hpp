#ifndef SOFTDSP_STATIC_CAST_HPP
#define SOFTDSP_STATIC_CAST_HPP

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
    template< typename To >
    struct static_cast_ {
      template< typename Context >
      class eval {
      public:
        eval( const Context &context_ ) : tools( context_.get_toolbox() ) {}
        eval( const typename Context::toolbox_type &tools_ ) : tools( tools_ ) {}
        template< typename ValueType >
        return_value< typename boost::remove_reference< To >::type >
        operator()(
          ValueType value_,
          typename boost::enable_if<
            at_least_one_operand_is_llvm_value< ValueType >
          >::type* = 0
        ) {
          const auto value = tools->as_llvm_value( tools->load( value_ ) );
          return return_value< typename boost::remove_reference< To >::type >(
            cast<
              typename remove_proxy< 
                typename boost::remove_reference< 
                  typename get_return_type< ValueType >::type
                >::type
              >::type,
              typename boost::remove_reference< To>::type
            >( value.value )
          );
        }
        template< typename ValueType >
        auto operator()(
          ValueType value_,
          typename boost::enable_if<
            boost::mpl::not_<
              at_least_one_operand_is_llvm_value< ValueType >
            >
          >::type* = 0
        ) -> To {
          return static_cast< To >( value_ );
        }
      private:
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_integral< From >,
              boost::is_integral< To_ >
            >
          >::type* = 0
        ) {
          return
            tools->ir_builder.CreateIntCast(
              src,
              tools->type_generator_( tag< To_ >() ),
              boost::is_signed< From >::value
            );
        }
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_float< From >,
              boost::is_float< To_ >
            >
          >::type* = 0
        ) {
          return
            tools->ir_builder.CreateFPCast(
              src,
              tools->type_generator_( tag< To_ >() )
            );
        }
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_float< From >,
              boost::is_integral< To_ >,
              boost::mpl::not_< boost::is_signed< To_ > >
            >
          >::type* = 0
        ) {
          return
            tools->ir_builder.CreateFPToUI(
              src,
              tools->type_generator_( tag< To_ >() )
            );
        }
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_float< From >,
              boost::is_integral< To_ >,
              boost::is_signed< To_ >
            >
          >::type* = 0
        ) {
          return
            tools->ir_builder.CreateFPToSI(
              src,
              tools->type_generator_( tag< To_ >() )
            );
        }
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_integral< From >,
              boost::mpl::not_< boost::is_signed< From > >,
              boost::is_float< To_ >
            >
          >::type* = 0
        ) {
          return
            tools->ir_builder.CreateUIToFP(
              src,
              tools->type_generator_( tag< To_ >() )
            );
        }
        template< typename From, typename To_ >
        llvm::Value *cast(
          llvm::Value *src,
          typename boost::enable_if<
            boost::mpl::and_<
              boost::is_integral< From >,
              boost::is_signed< From >,
              boost::is_float< To_ >
            >
          >::type* = 0
        ) {
          std::cout << "debug:::" << std::endl;
          src->getType()->dump();
          std::cout << "debug:::" << std::endl;
          return
            tools->ir_builder.CreateSIToFP(
              src,
              tools->type_generator_( tag< To_ >() )
            );
        }
        const typename Context::toolbox_type tools;
      };
    };
  }
  
  template< typename To, typename Expr >
  auto static_cast_( const Expr &expr ) ->
    decltype( std::declval< typename boost::proto::terminal< keywords::static_cast_< To > >::type >()( expr ) ){
    typename boost::proto::terminal< keywords::static_cast_< To > >::type static_cast__;
    return static_cast__( expr );
  }
}

#endif
