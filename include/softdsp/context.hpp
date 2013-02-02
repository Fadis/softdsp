#ifndef SOFTDSP_CONTEXT_HPP
#define SOFTDSP_CONTEXT_HPP

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
#include <softdsp/subscript.hpp>
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

  template< typename function_type_ >
  class softdsp_context {
  public:
    typedef function_type_ function_type;
    typedef std::shared_ptr< llvm_toolbox< function_type > > toolbox_type;
    typedef softdsp_context< function_type > context_type;
    typedef boost::function_types::result_type< function_type > result_type;
    typedef boost::function_types::parameter_types< function_type > parameter_types;
    softdsp_context(
      const toolbox_type &tools_
    ) : tools( tools_ ) {}
    template< typename Expr, typename Enable = void > struct eval{};
    template< typename Expr > struct eval<
      Expr,
      typename boost::enable_if<
        proto::matches< Expr, proto::terminal< proto::_ > >
      >::type
    > {
      typedef decltype( std::declval< context_type& >().tools->as_value( boost::proto::value( std::declval< Expr& >() ) ) ) result_type;
      result_type operator()( Expr &expr, context_type &context ) {
        return context.tools->as_value( boost::proto::value( expr ) );
      }
    };

SOFTDSP_ENABLE_UNARY_OPERATOR_( dereference )
SOFTDSP_ENABLE_BINARY_OPERATOR_( plus )
SOFTDSP_ENABLE_BINARY_OPERATOR_( minus )
SOFTDSP_ENABLE_BINARY_OPERATOR_( subscript )
    const toolbox_type &get_toolbox() const {
      return tools;
    }
  private:
    const toolbox_type tools;
  };
}

#endif
