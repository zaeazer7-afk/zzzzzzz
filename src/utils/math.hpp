#pragma once

#include <cmath>
#include <algorithm>

namespace Utils
{
    constexpr float PI = 3.14159265358979323846f;

    inline float DegToRad( float deg ) { return deg * ( PI / 180.f ); }
    inline float RadToDeg( float rad ) { return rad * ( 180.f / PI ); }
    inline float Clamp( float v, float lo, float hi ) { return v < lo ? lo : ( v > hi ? hi : v ); }
    inline float Lerp( float a, float b, float t ) { return a + ( b - a ) * t; }

    struct Vec2
    {
        float x = 0, y = 0;

        Vec2() = default;
        Vec2( float x, float y ) : x( x ), y( y ) {}

        Vec2 operator+( const Vec2& o ) const { return { x + o.x, y + o.y }; }
        Vec2 operator-( const Vec2& o ) const { return { x - o.x, y - o.y }; }
        Vec2 operator*( float s ) const { return { x * s, y * s }; }
        Vec2 operator/( float s ) const { return { x / s, y / s }; }

        float Length() const { return sqrtf( x * x + y * y ); }
        float Dot( const Vec2& o ) const { return x * o.x + y * o.y; }
        float Distance( const Vec2& o ) const { return ( *this - o ).Length(); }
        Vec2 Normalized() const { float l = Length(); return l > 0 ? *this / l : Vec2{}; }
    };

    struct Vec3
    {
        float x = 0, y = 0, z = 0;

        Vec3() = default;
        Vec3( float x, float y, float z ) : x( x ), y( y ), z( z ) {}

        Vec3 operator+( const Vec3& o ) const { return { x + o.x, y + o.y, z + o.z }; }
        Vec3 operator-( const Vec3& o ) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec3 operator*( float s ) const { return { x * s, y * s, z * s }; }
        Vec3 operator/( float s ) const { return { x / s, y / s, z / s }; }

        float Length() const { return sqrtf( x * x + y * y + z * z ); }
        float Length2D() const { return sqrtf( x * x + y * y ); }
        float Dot( const Vec3& o ) const { return x * o.x + y * o.y + z * o.z; }
        float Distance( const Vec3& o ) const { return ( *this - o ).Length(); }
        Vec3 Cross( const Vec3& o ) const { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }
        Vec3 Normalized() const { float l = Length(); return l > 0 ? *this / l : Vec3{}; }
    };

    struct Vec4
    {
        float x = 0, y = 0, z = 0, w = 0;
        Vec4() = default;
        Vec4( float x, float y, float z, float w ) : x( x ), y( y ), z( z ), w( w ) {}
    };

    // Angle between two 3D positions in degrees (yaw, pitch).
    inline Vec2 AngleTo( const Vec3& from, const Vec3& to )
    {
        Vec3 delta = to - from;
        float dist = delta.Length2D();
        float yaw  = RadToDeg( atan2f( delta.y, delta.x ) );
        float pitch = RadToDeg( -atan2f( delta.z, dist ) );
        return { yaw, pitch };
    }

    // 2D FOV — pixel distance from screen center to a screen point.
    inline float CalcFOV( const Vec2& screenCenter, const Vec2& screenPoint )
    {
        return screenCenter.Distance( screenPoint );
    }

    // Quadratic bezier interpolation for smooth aimbot.
    inline Vec2 BezierPoint( const Vec2& p0, const Vec2& p1, const Vec2& p2, float t )
    {
        float u  = 1.f - t;
        float tt = t * t;
        float uu = u * u;
        return p0 * uu + p1 * ( 2.f * u * t ) + p2 * tt;
    }

    // Smooth step for natural-looking interpolation.
    inline float SmoothStep( float edge0, float edge1, float x )
    {
        float t = Clamp( ( x - edge0 ) / ( edge1 - edge0 ), 0.f, 1.f );
        return t * t * ( 3.f - 2.f * t );
    }
}
