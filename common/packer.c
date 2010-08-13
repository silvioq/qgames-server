/****************************************************************************
 * Copyright (c) 2009-2010 Silvio Quadri                                    *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 ****************************************************************************/

#include  <stdio.h>
#include  <string.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <stdint.h>
#include  <time.h>
#include  <sys/time.h>
#include  "packer.h"

#define   ALLOC_SIZE 128
#define  ADDDATA( data, len, alloc, pointer, dato ){ \
    if( len + sizeof( dato ) > alloc ){ \
        alloc += ( len + sizeof( dato ) > alloc + ALLOC_SIZE ? sizeof( dato ) + ALLOC_SIZE : ALLOC_SIZE ); \
        data = realloc( data, alloc ); \
    } \
    *(typeof(dato)*)pointer = dato ;\
    pointer = ((char*)pointer) + sizeof( dato ) ;\
    len += sizeof( dato ); \
  }

#define  ADDBIN( data, len, alloc, pointer, dato, size ){ \
    if( len + size  > alloc ) { \
        alloc += ( len + size > alloc + ALLOC_SIZE ? size + ALLOC_SIZE : ALLOC_SIZE ); \
        data = realloc( data, alloc ); \
    } \
    memcpy( pointer, dato, size ); \
    pointer = ((char*)pointer) + size; \
    len += size; \
  }

/*
 * Dado un formato y una lista de elementos, devuelve en 
 * data el binario. En size estará el tamaño usado.
 *
 * Formato:
 *   c  => char
 *   h  => int (almacenado con 16 bits)
 *   i  => int (almacenado con 32 bits)
 *   l  => long (almacenado con 64 bits)
 *   s  => string (terminado en '\0')
 *   b  => Binario (primero informar puntero y luego puntero
 *         ej  binary_pack( "b", &target_data, &target_size, origin_data, origin_size )
 * */
int    binary_pack( char* format, void** data, int* size, ... ){

    va_list  vlist;
    va_start( vlist, size );
    int    allocado = ALLOC_SIZE;
    void*  ret = malloc( allocado );
    void*  point = ret;
    int    ret_size = 0;


    while( *format ){
        uint8_t  len8;
        uint16_t  len16;
        uint32_t  len32;
        uint64_t  len64;
        char*     str;
        void*     ptr;
        int       ss;
        switch(*format){
            case 'c':
                len8 = (uint8_t)va_arg( vlist, int );
                ADDDATA( ret, ret_size, allocado, point, len8 );
                break;
            case 'h':
                len16 = (uint16_t)va_arg( vlist, int );
                ADDDATA( ret, ret_size, allocado, point, len16 );
                break;
            case 'i':
                len32 = va_arg( vlist, int );
                ADDDATA( ret, ret_size, allocado, point, len32 );
                break;
            case 'l':
                len64 = va_arg( vlist, uint64_t );
                ADDDATA( ret, ret_size, allocado, point, len64 );
                break;
            case 's':
                str = va_arg( vlist, char* );
                ss = strlen( str ) + 1;
                ADDBIN( ret, ret_size, allocado, point, str, ss );
                break;
            case 'b':
                ptr = va_arg( vlist, void* );
                ss = va_arg( vlist, int );
                len32 = (uint32_t)ss;
                ADDDATA( ret, ret_size, allocado, point, len32 )
                ADDBIN( ret, ret_size, allocado, point, ptr, ss );
                break;
            default:
                va_end( vlist );
                free( ret );  // Limpio lo que hice hasta el momento
                return 0;     // y me voy con error
             }
        format ++;
    }

    *data = ret;
    *size = ret_size;
    va_end( vlist );
    return ret_size;

}


/*
 * Este es el "desempacador" de binario. Recibe un binario y 
 * pone en cada puntero pasado como parametro el valor que
 * corresoponda.
 * Formato:
 *   c  => char
 *   h  => int (almacenado con 16 bits)
 *   i  => int (almacenado con 32 bits)
 *   l  => long (almacenado con 64 bits)
 *   s  => char**
 *   b  => Binario (primero informar puntero y luego puntero
 *         ej  binary_pack( "b", packed_data, packed_size, &extracted_data , &extracted_size )
 *
 *
 * Tanto la extraccion de binarios como de strings, apuntan al propio
 * bloque de datos de entrada, por lo cual, es necesario tomar 
 * recaudos antes de liberar el bloque de memoria de datos.
 *
 * Retorno:
 *   0 => Error
 *   1 => Ok
 *  -n => Los ultimos n elementos no fueron seteados
 * */
int    binary_unpack( char* format, void* data, int size, ... ){
    va_list  vlist;
    va_start( vlist, size );
    void* pointer = data;
    char* last_byte = ((char*)pointer) + size;
    int  len = 0;
    int  resto = 0;
    
    while( *format ){
        char*     to_char;
        int*      to_int;
        long*     to_long;
        int       aux_size;
        char**    to_str;
        void**    to_ptr;
        switch( *format ){
            case 'c':
                to_char    = va_arg( vlist, char* );
                if( resto || ((char*)pointer) + sizeof(uint8_t) > last_byte ){ 
                    resto ++;
                    if( to_char ) *to_char = 0;
                } else {
                    if( to_char ) *to_char = (char)(((uint8_t*)pointer)[0]);
                    pointer   += sizeof(uint8_t);
                }
                break;
            case 'h':
                to_int    = va_arg( vlist, int* );
                if( resto || ((char*)pointer) + sizeof(uint16_t) > last_byte ){ 
                    resto ++;
                    if( to_int ) *to_int = 0;
                } else {
                    if( to_int ) to_int[0] = (int)(((uint16_t*)pointer)[0]);
                    pointer   += sizeof(uint16_t);
                }
                break;
            case 'i':
                to_int    = va_arg( vlist, int* );
                if( resto || ((char*)pointer) + sizeof(uint32_t) > last_byte ){ 
                    resto ++;
                    if( to_int ) *to_int = 0;
                } else {
                    if( to_int ) *to_int = (int)(((uint32_t*)pointer)[0]);
                    pointer   += sizeof(uint32_t);
                }
                break;
            case 'l':
                to_long    = va_arg( vlist, long* );
                if( resto || ((char*)pointer) + sizeof(uint64_t) > last_byte ){ 
                    resto ++;
                    if( to_long ) *to_long = 0;
                } else {
                    if( to_long) *to_long = (long)(((uint64_t*)pointer)[0]);
                    pointer   += sizeof(uint64_t);
                }
                break;
            case 's':
                to_str    = va_arg( vlist, char** );
                if( resto ){
                    resto ++;
                    if( to_str ) *to_str = NULL;
                } else {
                    aux_size = strlen( ((char*)pointer ) ); 
                    if( ((char*)pointer) + aux_size > last_byte ){
                        resto ++;
                        if( to_str ) *to_str = NULL;
                    } else {
                        if( to_str ) *to_str = ((char*)pointer);
                        pointer   += aux_size + 1;
                    }
                }
                break;
            case 'b':
                to_ptr    = va_arg( vlist, void** );
                to_int    = va_arg( vlist, int* );
                if( resto || ((char*)pointer) + sizeof(uint32_t) > last_byte ){
                    resto ++;
                    if( to_int ) *to_int = 0;
                    if( to_ptr ) *to_ptr = NULL;
                    break;
                }
                aux_size  = (int)(((uint32_t*)pointer)[0]);
                if( aux_size == 0 ){
                    if( to_int ) *to_int = 0;
                    if( to_ptr ) *to_ptr = NULL;
                    break;
                }
                if( to_int ) *to_int = aux_size;
                pointer   += sizeof(uint32_t);
                if( ((char*)pointer) + aux_size > last_byte ){
                    resto ++;
                    if( to_ptr ) *to_ptr = NULL;
                    break;
                }
                if( to_ptr ) *to_ptr = pointer;
                pointer   += aux_size;
                break;
            default:
                va_end( vlist );
                return 0;
        }
        format ++;
                
    }

    va_end( vlist );
    return ( resto ? - resto : 1 );
}
