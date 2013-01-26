#ifndef SOFTDSP_DATA_LAYOUT_LOAD_VALUE_AT_HPP
#define SOFTDSP_DATA_LAYOUT_LOAD_VALUE_AT_HPP

#include <boost/range/algorithm.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/spirit/include/qi.hpp>

namespace softdsp {
  namespace data_layout {
  template< typename T > 
  T load_value_at(
    uint8_t *at, size_t element_size, bool,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int8_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint8_t >
      >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( qi::parse( at, at + element_size, qi::byte_, result ) ) return result;
    else throw -1;
  }


  template< typename T > 
  T load_value_at(
    uint8_t *at, size_t element_size, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int16_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint16_t >
      >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( is_big_endian ) {
      if( qi::parse( at, at + element_size, qi::big_word, result ) ) return result;
      else throw -1;
    }
    else {
      if( qi::parse( at, at + element_size, qi::little_word, result ) ) return result;
      else throw -1;
    }
  }

  template< typename T > 
  T load_value_at(
    uint8_t *at, size_t element_size, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int32_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint32_t >
      >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( is_big_endian ) {
      if( qi::parse( at, at + element_size, qi::big_dword, result ) ) return result;
      else throw -1;
    }
    else {
      if( qi::parse( at, at + element_size, qi::little_dword, result ) ) return result;
      else throw -1;
    }
  }

  template< typename T > 
  T load_value_at(
    uint8_t *at, size_t element_size, bool is_big_endian,
    typename boost::enable_if<
      boost::mpl::or_<
        boost::is_same< typename boost::remove_cv< T >::type, int64_t >,
        boost::is_same< typename boost::remove_cv< T >::type, uint64_t >
      >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( is_big_endian ) {
      if( qi::parse( at, at + element_size, qi::big_qword, result ) ) return result;
      else throw -1;
    }
    else {
      if( qi::parse( at, at + element_size, qi::little_qword, result ) ) return result;
      else throw -1;
    }
  }

  template< typename T > 
  T load_value_at(
    const uint8_t *at, size_t element_size, bool is_big_endian,
    typename boost::enable_if<
      boost::is_same< typename boost::remove_cv< T >::type, float >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( is_big_endian ) {
      if( qi::parse( at, at + element_size, qi::big_bin_float, result ) ) return result;
      else throw -1;
    }
    else {
      if( qi::parse( at, at + element_size, qi::little_bin_float, result ) ) return result;
      else throw -1;
    }
  }

  template< typename T > 
  T load_value_at(
    const uint8_t *at, size_t element_size, bool is_big_endian,
    typename boost::enable_if<
      boost::is_same< typename boost::remove_cv< T >::type, double >
    >::type* = 0
  ) {
    namespace qi = boost::spirit::qi;
    T result;
    if( is_big_endian ) {
      if( qi::parse( at, at + element_size, qi::big_bin_double, result ) ) return result;
      else throw -1;
    }
    else {
      if( qi::parse( at, at + element_size, qi::little_bin_double, result ) ) return result;
      else throw -1;
    }
  }
  }
}
#endif
