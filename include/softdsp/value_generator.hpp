#ifndef SOFTDSP_VALUE_GENERATOR_GENERATOR_HPP
#define SOFTDSP_VALUE_GENERATOR_GENERATOR_HPP

#include <llvm/ADT/ArrayRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/Constants.h>
#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>
#include <array>
#include <memory>
#include <vector>
#include <boost/array.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/boost_array.hpp>
#include <boost/fusion/include/copy.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/adapted/adt/adapt_adt.hpp>
#include <boost/fusion/mpl.hpp>

#include <hermit/range_traits.hpp>
#include <softdsp/static_range_size.hpp>

#include <cxxabi.h>


namespace softdsp {
  class value_generator {
  public:
    value_generator(
      const std::shared_ptr< llvm::LLVMContext > &context_,
      const std::string &layout_
    ) : context( context_ ), layout( layout_ ) {}
    template< typename T >
    T operator()( tag< T > ) {
      const std::shared_ptr< data_layout::common > common_resources_(
        new data_layout::common( context, layout, tag< T >() )
      );
      return T(
        common_resources_,
        boost::iterator_range< uint8_t* >(
          common_resources_->data.data(),
          common_resources_->data.data() + common_resources_->data.size() 
        )
      );
    }
  private:
    const std::shared_ptr< llvm::LLVMContext > context;
    const std::string layout;
  };
}

#endif
