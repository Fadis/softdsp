#ifndef SOFTDSP_DATA_LAYOUT_PROXY_HPP
#define SOFTDSP_DATA_LAYOUT_PROXY_HPP

#include <softdsp/data_layout/load_value_at.hpp>
#include <softdsp/data_layout/store_value_at.hpp>
#include <softdsp/data_layout/common.hpp>

#include <memory>
#include <boost/range/algorithm.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/fusion/algorithm.hpp>

namespace softdsp {
  namespace data_layout {
    template< typename T >
    class proxy {
    public:
      proxy(
        const std::shared_ptr< common > &common_resources_,
        const boost::iterator_range< uint8_t* > &range_
      ) : common_resources( common_resources_ ), range( range_ ) {}
      proxy( const proxy< T > &src ) :
        common_resources( src.common_resources ), range( src.range ) {}
      proxy( proxy< T > &&src ) :
        common_resources( src.common_resources ), range( src.range ) {}
      operator T () const {
        return load_value_at< T >( boost::begin( range ), boost::distance( range ), common_resources->layout.isBigEndian() );
      }
      proxy &operator=( const T &value ) {
        store_value_at( value, boost::begin( range ), common_resources->layout.isBigEndian() );
        return *this; 
      }
      proxy &operator=( T &&value ) {
        store_value_at( value, boost::begin( range ), common_resources->layout.isBigEndian() );
        return *this; 
      }
    private:
      const std::shared_ptr< common > common_resources;
      const boost::iterator_range< uint8_t* > range;
    };
  }
}

#endif
