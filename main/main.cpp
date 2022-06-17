// todo ive put the fpm folder in program files!
#include <fpm/fixed.hpp>  // For fpm::fixed_16_16
#include <fpm/math.hpp>   // For fpm::cos
#include <fpm/ios.hpp>    // For fpm::operator<<

#include <iostream>
#include <chrono>

const uint64_t MAX_FIPO_BOUND = 0x000003ffffffffff;

struct fipo64
{
    bool sign;
    bool did_overflow;
    uint64_t value;
    uint64_t guard;
};

struct fipo_21_21
{
    bool sign;
    bool did_overflow;
    uint64_t value;
    uint64_t guard;
};



fipo64 from_int32(int32_t val)
{
    fipo64 ans;
    constexpr uint32_t ugly = 1;
    ans.sign = val & (ugly << 31);
    ans.value = val;
    ans.value <<= 32;
    ans.guard = 0;
    ans.did_overflow = false;
    return ans;


}

// todo little/big endian!
fipo64 mul(fipo64 x, fipo64 y)
{
    const uint64_t msb_x = (x.value & 0xFFFFFFFF00000000) >> 32;
    const uint64_t msb_y = (y.value & 0xFFFFFFFF00000000) >> 32;
    fipo64 ans{ true, false,0,0 };
    uint64_t ans_64 = msb_x * msb_y;
    ans.guard = ans_64 >> 32;
    // todo maybe even assert this
    if (ans.guard > 0)
    {
        ans.did_overflow = true;
        return ans;
    }
    ans.guard = 0;
    const uint64_t lsb_x = (x.value & 0x00000000FFFFFFFF);
    const uint64_t lsb_y = (y.value & 0x00000000FFFFFFFF);
    ans_64 = lsb_x * lsb_y;
    ans.guard = ans_64 << 32;
    ans.value = ans_64 >> 32 | ans.value;
    ans.sign = (x.sign ^ y.sign) ? false : true;
    return ans;

}

// todo little/big endian!
fipo64 mul2(fipo64 x, fipo64 y)
{
    const uint64_t msb_x = (x.value & 0xFFFFFFFF00000000) >> 32;
    const uint64_t msb_y = (y.value & 0xFFFFFFFF00000000) >> 32;
    fipo64 ans{ true, false,0,0 };
    uint64_t ans_64 = msb_x * msb_y;
    ans.guard = ans_64 >> 32;
    // todo maybe even assert this
    if (ans.guard > 0)
    {
        ans.did_overflow = true;
        return ans;
    }
    ans.guard = 0;
    const uint64_t lsb_x = (x.value & 0x00000000FFFFFFFF);
    const uint64_t lsb_y = (y.value & 0x00000000FFFFFFFF);
    ans_64 = lsb_x * lsb_y;
    ans.guard = ans_64 << 32;
    ans.value = ans_64 >> 32 | ans.value;
    //
    ans_64 = msb_x * lsb_y + msb_y * lsb_x;
    ans.value += ans_64;
    ans.sign = (x.sign ^ y.sign) ? false : true;
    return ans;

}
const uint32_t MAX_FIPO_I_BOUND = 0x1FFFFF;

fipo_21_21 from_uint32_t(uint32_t x)
{
    if(x>MAX_FIPO_I_BOUND)
    {
        throw "from_uint32_t:: given x is too big, max value is 0x1FFFFF.";
    }
    fipo_21_21 ans;
    ans.value = x;
    ans.value <<= 21;
    ans.guard = 0;
    ans.sign = true;
    ans.did_overflow = false;
    return ans;
	
}

// double conversions are for debug purposes only!
fipo_21_21 from_double(double x)
{
    double temp = x * (pow(2, 21));
    temp = round(temp);
    if (temp>MAX_FIPO_BOUND)
    {
        throw "double x is too big!";
    }
    fipo_21_21 res{ signbit(x), false, static_cast<uint64_t>(temp), 0 };
    return res;

}
// double conversions are for debug purposes only!
double to_double(fipo_21_21 x)
{
    // uint64_t moved = x.value << 21;
    double a = x.value / (pow(2, 21));
    a *= x.sign ? 1 : -1;
    return a;
}

fipo_21_21 mul_saturated(const fipo_21_21 x, const fipo_21_21 y)
{
    fipo_21_21 ans{ true, false,MAX_FIPO_BOUND,0 };
    uint64_t ans_64 = x.value * y.value;
    ans.sign = (x.sign ^ y.sign) ? false : true;
    // check "bad" overflow
    if(y.value!=0 && ans_64/y.value!=x.value)
    {
	    //overflow
        ans.did_overflow = true;
        // ans.value = MAX_FIPO_BOUND;
        return ans;
    }
    
    // todo, design chםice: should we add lost bits to guard?
    ans.guard = ans_64 << (64 - 21);
    ans.value = ans_64 >> 21;
    // check "good" overflow
    const uint64_t GUARD_MASK = 0xFFFFFC0000000000;
    // 1111 1111 1111 1111 1111 1100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000
    if ((ans.value & GUARD_MASK) > 0)
    {
        //overflow
        ans.did_overflow = true;
        ans.value = MAX_FIPO_BOUND;
        return ans;
    }
    // check underflow
    if(ans.guard>0 && ans.value == 0)
    {
        ans.did_overflow = true;
        ans.value = 1;
        return ans;
    }
    
    return ans;

}

const uint64_t ITERATIONS = 29639696;
int main(int argc, char* argv[])
{
    double e = DBL_EPSILON;
    //todo read about volatile.
    volatile double x = 2.0;
    volatile double almost1 = 1.0 + e;
    auto begin = std::chrono::high_resolution_clock::now();
    uint64_t i = 0;
    for (; i < ITERATIONS; i++)
    {
        x = x * almost1;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "test1: " << elapsed.count() << " i: " << i << " x: " << x;

    //-----
    fipo_21_21 z = from_uint32_t(2);
    fipo_21_21 alm1;
    alm1 = from_uint32_t(1);
    alm1.value += 1;
    std::cout << mul_saturated(z, z).value;
    begin = std::chrono::high_resolution_clock::now();
    i = 0;
    for (; i < ITERATIONS; i++)
    {
        z = mul_saturated(z, alm1);
        if (z.did_overflow)
        {
            std::cout << "\noverflow: " << i << " z: " << z.value;
            break;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "\ntest2: " << elapsed.count() << " i: " << i << " z: " << z.value;
    //----
    fipo_21_21 max_f; max_f.value = MAX_FIPO_BOUND;
    begin = std::chrono::high_resolution_clock::now();
    i = 0;
    for (; i < ITERATIONS; i++)
    {
        max_f = mul_saturated(max_f, max_f);
    }
    end = std::chrono::high_resolution_clock::now();
    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    std::cout << "\n overflow test: " << elapsed.count() << " i: " << i << " max f: " << max_f.value;
    //---
    std::cout << "\n correctness test: ";
    i = 0;
    fipo_21_21 w = from_uint32_t(2);
    double wd = 2.0;
    for (;i<100;i++)
    {
	    std::cout << "\ndouble*i = " << wd*i << "|fipo*i = " << to_double(mul_saturated(w, from_uint32_t(i)));
    }
}
