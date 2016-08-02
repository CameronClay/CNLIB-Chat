#pragma once

template <typename T>
class _Vec2
{
public:
	inline _Vec2() {}
	inline _Vec2( T x,T y )
		:
	x( x ),
	y( y )
	{}
	inline _Vec2( const _Vec2& vect )
		:
		_Vec2( vect.x,vect.y )
	{}
	template <typename T2>
	inline operator _Vec2< T2 >() const
	{
		return { (T2)x,(T2)y };
	}
	inline T LenSq() const
	{
		return *this * *this;
	}
	inline T Len() const
	{
		return sqrt( LenSq() );
	}
	inline _Vec2& Normalize()
	{
		const T length = Len();
		x /= length;
		y /= length;
		return *this;
	}
	inline _Vec2& CCW90()
	{
		T temp = y;
		y = -x;
		x = temp;
		return *this;
	}
	inline _Vec2& CW90()
	{		
		T temp = y;
		y = x;
		x = -temp;
		return *this;
	}
	inline _Vec2& Swap( _Vec2& vect )
	{
		const _Vec2 temp = vect;
		vect = *this;
		*this = temp;
		return *this;
	}
	inline _Vec2 operator-() const
	{
		return _Vec2( -x,-y );
	}
	inline _Vec2& operator=( const _Vec2 &rhs )
	{
		this->x = rhs.x;
		this->y = rhs.y;
		return *this;
	}
	inline _Vec2& operator+=( const _Vec2 &rhs )
	{
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}
	inline _Vec2& operator-=( const _Vec2 &rhs )
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		return *this;
	}
	inline T operator*( const _Vec2 &rhs ) const
	{
		return this->x * rhs.x + this->y * rhs.y;
	}	
	inline _Vec2 operator+( const _Vec2 &rhs ) const
	{
		return _Vec2( *this ) += rhs;
	}
	inline _Vec2 operator-( const _Vec2 &rhs ) const
	{
		return _Vec2( *this ) -= rhs;
	}
	inline _Vec2& operator*=( const T &rhs )
	{
		this->x *= rhs;
		this->y *= rhs;
		return *this;
	}
	inline _Vec2 operator*( const T &rhs ) const
	{
		return _Vec2( *this ) *= rhs;
	}
	inline _Vec2& operator/=( const T &rhs )
	{
		this->x /= rhs;
		this->y /= rhs;
		return *this;
	}
	inline _Vec2 operator/( const T &rhs ) const
	{
		return _Vec2( *this ) /= rhs;
	}
	inline T CrossWith( const _Vec2 &rhs ) const
	{
		return x * rhs.y - y * rhs.x;
	}
	inline bool	IsInsideRect( const _Vec2 &p1,const _Vec2 & p2 ) const
	{
		const T medianX = (p1.x + p2.x) / 2.0f;
		const T medianY = (p1.y + p2.y) / 2.0f;
		return abs( medianX - x ) <= abs( medianX - p1.x ) &&
			abs( medianY - y) <= abs( medianY - p1.y );
	}
	inline bool	operator==( const _Vec2 &rhs ) const
	{
		return x == rhs.x && y == rhs.y;
	}
	inline bool	operator!=(const _Vec2 &rhs) const
	{
		return !(*this == rhs);
	}
public:
	T x, y;
};

typedef _Vec2< float > Vec2;
