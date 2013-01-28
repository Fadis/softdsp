#ifndef SOFTDSP_DATA_LAYOUT_COMMON_HPP
#define SOFTDSP_DATA_LAYOUT_COMMON_HPP

#include <softdsp/type_generator.hpp>

#include <llvm/LLVMContext.h>
#include <llvm/DataLayout.h>
#include <memory>
#include <vector>

#include <softdsp/type_generator.hpp>

namespace softdsp {
  namespace data_layout {
    struct common {
      common(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_,
        size_t size_
      ) :
        context( context_ ), layout( layout_ ), type_generator_( context ), data( size_ ) {}
      template< typename T >
      common(
        const std::shared_ptr< llvm::LLVMContext > &context_,
        const std::string &layout_,
        const tag< T > &tag_
      ) :
        context( context_ ), layout( layout_ ), type_generator_( context ), data( layout.getTypeAllocSize( type_generator_( tag_ ) ) ) {}
      const std::shared_ptr< llvm::LLVMContext > context;
      const llvm::DataLayout layout;
      const type_generator type_generator_; 
      std::vector< uint8_t > data;
    };
  }
}

#endif
