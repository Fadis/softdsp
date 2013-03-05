#ifndef SOFTDSP_LOGICAL_NOT_HPP
#define SOFTDSP_LOGICAL_NOT_HPP

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
#include <softdsp/context_definitions.hpp>
#include <softdsp/return_value.hpp>
#include <softdsp/binalize.hpp>

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
#include <boost/type_traits.hpp> 

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>

namespace softdsp {
  namespace proto = boost::proto;
  using proto::argsns_::list2;
  using proto::exprns_::expr;

  namespace keywords {
    template< typename Context >
    class logical_not {
    public:
      logical_not( const Context &context_ ) : tools( context_.get_toolbox() ) {}
      logical_not( const typename Context::toolbox_type &tools_ ) : tools( tools_ ) {}
      template< typename ValueType >
      return_value< bool >
      operator()(
        ValueType value_,
        typename boost::enable_if<
          boost::mpl::and_<
            at_least_one_operand_is_llvm_value< ValueType >,
            boost::is_integral<
              typename boost::remove_reference<
                typename get_return_type< ValueType >::type
              >::type
            >
          >
        >::type* = 0
      ) {
        binalize::template eval< Context > binalize_( tools );
        return return_value< bool >(
          tools->ir_builder.CreateNot( binalize_( value_ ).value )
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
      ) -> decltype( !std::declval< ValueType >() ) {
        return !value_;
      }
    private:
      const typename Context::toolbox_type tools;
    };
  }
}

#endif
