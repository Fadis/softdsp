#ifndef SOFTDSP_USUAL_ARITHMETIC_CONVERSIONS_HPP
#define SOFTDSP_USUAL_ARITHMETIC_CONVERSIONS_HPP

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
#include <softdsp/static_cast.hpp>

#include <cxxabi.h>

namespace softdsp {
  template<
    typename LeftType,
    typename RightType,
    bool is_primitive =
      boost::mpl::and_<
        is_primitive<
          typename boost::remove_reference<
            typename remove_proxy<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >::type
          >::type
        >,
        is_primitive<
          typename boost::remove_reference<
            typename remove_proxy<
              typename boost::remove_reference<
                typename get_return_type< RightType >::type
              >::type
            >::type
          >::type
        >
      >::value
  > struct usual_arithmetic_conversions {};

  template<
    typename LeftType,
    typename RightType
  > struct usual_arithmetic_conversions< LeftType, RightType, true > {
    typedef
      decltype(
        std::declval<
          typename boost::remove_reference<
            typename remove_proxy<
              typename boost::remove_reference<
                typename get_return_type< LeftType >::type
              >::type
            >::type
          >::type
        >() +
        std::declval<
          typename boost::remove_reference<
            typename remove_proxy<
              typename boost::remove_reference<
                typename get_return_type< RightType >::type
              >::type
            >::type
          >::type
        >()
      ) type;
  };
}

#endif
