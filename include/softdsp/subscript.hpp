#ifndef SOFTDSP_SUBSCRIPT_HPP
#define SOFTDSP_SUBSCRIPT_HPP

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
    template< typename Context >
    class subscript {
    public:
      subscript( const Context &context_ ) : tools( context_.get_toolbox() ) {}
      template< typename LeftType, typename RightType >
      return_value<
        typename hermit::range_value<
          typename boost::remove_reference<
            typename get_return_type< LeftType >::type
          >::type
        >::type
      >
      operator()(
        LeftType left_,
        RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            at_least_one_operand_is_llvm_value< LeftType, RightType >,
            hermit::is_forward_traversal_range<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >,
            is_primitive<
              typename hermit::range_value<
                typename boost::remove_reference<
                  typename get_return_type< LeftType >::type
                >::type
              >::type
            >
          >
        >::type* = 0
      ) {
        const auto left = tools->as_llvm_value( tools->load( left_ ) );
        const auto right = tools->as_llvm_value( tools->load( right_ ) );
        return return_value<
          typename hermit::range_value<
            typename boost::remove_reference<
              typename get_return_type< LeftType >::type
            >::type
          >::type
        >(
          tools->ir_builder.CreateExtractElement( left.value, right.value )
        );
      }
      template< typename LeftType, typename RightType >
      return_value<
        typename hermit::range_value<
          typename boost::remove_reference<
            typename get_return_type< LeftType >::type
          >::type
        >::type
      >
      operator()(
        LeftType left_,
        RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            at_least_one_operand_is_llvm_value< LeftType, RightType >,
            hermit::is_forward_traversal_range<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >,
            boost::mpl::not_<
              is_primitive<
                typename hermit::range_value<
                  typename boost::remove_reference<
                    typename get_return_type< LeftType >::type
                  >::type
                >::type
              >
            >
          >
        >::type* = 0
      ) {
        const auto left = tools->as_llvm_value( tools->load( left_ ) );
        std::vector< unsigned int > args = { static_cast< unsigned int >( right_ ) };
        llvm::ArrayRef< unsigned int > args_ref( args );
        return return_value<
          typename hermit::range_value<
            typename boost::remove_reference<
              typename get_return_type< LeftType >::type
            >::type
          >::type
        >(
          tools->ir_builder.CreateExtractValue( left.value, args_ref )
        );
      }
      template< typename LeftType, typename RightType >
      return_value<
        typename boost::remove_pointer<
          typename boost::remove_reference<
            typename get_return_type< LeftType >::type
          >::type
        >::type
      >
      operator()(
        LeftType left_, RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            at_least_one_operand_is_llvm_value< LeftType, RightType >,
            boost::is_pointer<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >
            >
          >
        >::type* = 0
      ) {
        const auto left = tools->as_llvm_value( tools->load( left_ ) );
        const auto right = tools->as_llvm_value( tools->load( right_ ) );
        if( left.value->getType()->isPointerTy() ) {
          return return_value<
            typename boost::remove_pointer<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >::type
          >(
            tools->ir_builder.CreateLoad(
              tools->ir_builder.CreateAdd(
                left.value,
                mul(
                  right.value,
                  tools->data_layout->getTypeAlllocSize(
                    left.value->getType()
                  )
                )
              )
            )
          );
        }
        else
          throw -1;
      }
      template< typename LeftType, typename RightType >
      typename hermit::range_value<
        typename boost::remove_reference<
          typename get_return_type< LeftType >::type
        >::type
      >::type
      operator()(
        LeftType left_, RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            boost::mpl::not_<
              at_least_one_operand_is_llvm_value< LeftType, RightType >
            >,
            hermit::is_forward_traversal_range<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >
          >
        >::type* = 0
      ) {
        return *std::next( boost::begin( left_ ), right_ );
      }
      template< typename LeftType, typename RightType >
      auto operator()(
        LeftType left_, RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            boost::mpl::not_<
              at_least_one_operand_is_llvm_value< LeftType, RightType >
            >,
            boost::is_pointer<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >
            >
          >
        >::type* = 0
      ) -> decltype( left_[ right_ ] ) {
        return left_[ right_ ];
      }

   /* 
      template< typename LeftType, typename RightType >
      return_value<
        typename hermit::range_value<
          typename boost::remove_reference<
            typename get_return_type< LeftType >::type
          >::type
        >::type&
      >
      operator()(
        LeftType left_,
        RightType right_,
        typename boost::enable_if<
          boost::mpl::and_<
            at_least_one_operand_is_llvm_value< LeftType, RightType >,
            hermit::is_forward_traversal_range<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >
            >,
            boost::is_reference< typename get_return_type< LeftType >::type >
          >
        >::type* = 0
      ) {
        const auto left = tools->as_llvm_value( tools->load( left_ ) );
        const auto right = tools->as_llvm_value( tools->load( right_ ) );
        int status;
        std::cout << abi::__cxa_demangle( typeid( left ).name(), 0, 0, &status ) << std::endl;
        left.value->getType()->dump();
        return return_value<
          typename hermit::range_value<
            typename boost::remove_reference<
              typename get_return_type< LeftType >::type
            >::type
          >::type&
        >(
          tools->ir_builder.CreateGEP( left.value, right.value )
        );
      }
      */
    private:
      const typename Context::toolbox_type tools;
    };
  }
}

#endif
