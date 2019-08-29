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

#ifndef ATLASGEN_BOX_H_INCLUDED__
#define ATLASGEN_BOX_H_INCLUDED__

#include <vector>

namespace binpack {

template< typename T >
struct box {
    T width() const {
        return width_;
    }

    T height() const {
        return height_;
    }

    T x() const {
        return x_;
    }

    T y() const {
        return y_;
    }

    T top() const {
        return y_ + height_;
    }

    T right() const {
        return x_ + width_;
    }

    T left() const {
        return x();
    }

    T bottom() const {
        return y();
    }

    T area() const {
        return width() * height();
    }

    void scale( T val ) {
        x_ *= val;
        y_ *= val;
        width_  *= val;
        height_ *= val;
    }

    T x_, y_, width_, height_;
};

using boxd = box< double >;

bool overlap( const box<size_t>& a, const box<size_t>& b, size_t spacing )
{
    return !(a.right() + spacing <= b.left() || b.right() + spacing <= a.left() || a.top() + spacing <= b.bottom() || b.top() + spacing <= a.bottom());
}

void make_splits( box<size_t> a, box<size_t> b, std::vector< box< size_t > >& result, size_t spacing )
{
    result.clear();

    if( a.x() + spacing < b.x() ) {
        result.push_back( box< size_t >{ a.x(), a.y(), b.x() - a.x() - spacing, a.height() } );
    }

    if( a.right() > b.right() + spacing ) {
        result.push_back( box< size_t >{ b.right() + spacing, a.bottom(), a.right() - b.right() - spacing, a.height() } );
    }

    if( a.top() > b.top() + spacing ) {
        result.push_back( box< size_t >{ a.left(), b.top() + spacing, a.width(), a.top() - b.top() - spacing } );
    }

    if( a.bottom() + spacing < b.bottom() ) {
        result.push_back( box< size_t >{ a.left(), a.bottom(), a.width(), b.bottom() - a.bottom() - spacing } );
    }
}

bool can_fit( const box<size_t>& a, const box<size_t>& b )
{
    return a.width() >= b.width() && a.height() >= b.height();
}

bool contains( const box<size_t>& a, const box<size_t>& b )
{
    return b.left() >= a.left() && b.bottom() >= a.bottom() && b.right() <= a.right() && b.top() <= a.top();
}

bool operator==( const box<size_t>& a, const box<size_t>& b )
{
    return a.x_ == b.x_ && a.y_ == b.y_ && a.width_ == b.width_ && a.height_ == b.height_;
}

}

#endif
