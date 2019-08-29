// MIT License
//
// Copyright( c ) 2016 Michael Steinberg
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BINPACKING_H_INCLUDED__
#define BINPACKING_H_INCLUDED__

#include <vector>
#include <algorithm>
#include <iostream>
#include "box.h"

namespace binpack {

template< typename T >
T score_bssf( box<T>& a, box<T>& b )
{
    return std::min( a.width() - b.width(), a.height() - b.height() );
}

// http://clb.demon.fi/files/RectangleBinPack.pdf
// MAX-RECTANGLES-BSSF-BBF GLOBAL
template< typename T >
bool bin_pack_max_rect( std::vector< box<T>* >& input, T width, T height, T spacing )
{
    std::vector<box<T>> boxes;
    boxes.reserve( width*height );
    boxes.push_back( box< size_t >{ 0, 0, width, height } );

    std::vector< size_t > candidates;
    candidates.reserve( 100 );

    std::vector<box<T>> newrects;
    newrects.reserve( 4 );

    while( input.size() ) {
        if( input.size() % 50 == 0 ) {
            std::cout << '.';
        }

        // Find best source-dest pair (GLOBAL)
        size_t min_source = 0;
        size_t min_dest = 0;
        T min_score = std::max( width, height );
        bool found = false;

        for( unsigned int source_i = 0; source_i < input.size(); ++source_i ) {
            for( unsigned int dest_i = 0; dest_i < boxes.size(); ++dest_i ) {
                if( can_fit( boxes[dest_i], *input[source_i] ) ) {
                    auto score = score_bssf( boxes[dest_i], *input[source_i] );
                    if( score < min_score ) {
                        min_score = score;
                        min_source = source_i;
                        min_dest = dest_i;
                        found = true;
                    }
                }
            }
        }

        if( !found ) {
            std::cout << "bin packing failed with " << input.size() << " characters left to be placed.\n";
            return false;
        }

        // split dest rect
        auto& irect = *input[min_source];
        auto& drect = boxes[min_dest];

        irect.x_ = drect.x_;
        irect.y_ = drect.y_;

        make_splits( drect, irect, newrects, spacing );
        boxes.erase( boxes.begin() + min_dest );
        size_t size = boxes.size();

        for( auto& box : newrects ) {
            boxes.push_back( box );
        }

        // split all existing overlapping boxes with irect
        for( size_t i = 0; i<size; ) {
            if( overlap( boxes[i], irect, spacing ) ) {
                make_splits( boxes[i], irect, newrects, spacing );
                boxes.erase( boxes.begin() + i );

                for( auto& box : newrects ) {
                    boxes.push_back( box );
                }
                --size;
            }
            else {
                ++i;
            }
        }

        // find bogus rects
        candidates.clear();
        for( size_t i = 0; i<boxes.size(); ++i ) {
            for( size_t j = 0; j<boxes.size(); ++j ) {
                if( i != j ) {
                    if( boxes[i] == boxes[j] ) {
                        if( i > j ) candidates.push_back( i );
                    }
                    else if( contains( boxes[i], boxes[j] ) ) {
                        candidates.push_back( j );
                    }
                }
            }
        }

        std::sort( candidates.begin(), candidates.end(), std::less<size_t>() );
        candidates.erase( std::unique( candidates.begin(), candidates.end() ), candidates.end() );

        for( size_t removes = 0; removes < candidates.size(); ++removes ) {
            boxes.erase( boxes.begin() + (candidates[removes] - removes) );
        }

        input.erase( input.begin() + min_source );
    }

    std::cout << "\n";

    return true;
}



}

#endif
