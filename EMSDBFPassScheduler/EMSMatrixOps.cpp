// Matrix operations that were previously implemented in SIGPROC.CPP using
// ippmInvert_m_32f / ippmInvert_m_64f (removed in oneAPI IPP 2022+).
// emssbMatrixInverse / emssbMatrixInverseD now use Gauss-Jordan elimination
// with partial pivoting.  All other functions are pure C.

#include <math.h>
#include <new>
#include "sigproc.h"
#include "emsexcpt.h"

// ---------------------------------------------------------------------------

static void _gjInvert32f( const float *src, float *dst, int n )
{
    int w = 2 * n;
    float *aug = new (std::nothrow) float[ n * w ];
    if ( !aug ) throw CEMSException( EMS_EXCEPTION );

    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) aug[ i*w + j ] = src[ i*n + j ];
        for ( int j = n; j < w; j++ ) aug[ i*w + j ] = ( (j-n) == i ) ? 1.0f : 0.0f;
    }

    for ( int col = 0; col < n; col++ ) {
        int pivot = col;
        for ( int row = col+1; row < n; row++ )
            if ( fabs(aug[row*w+col]) > fabs(aug[pivot*w+col]) ) pivot = row;
        for ( int j = 0; j < w; j++ ) {
            float t = aug[col*w+j]; aug[col*w+j] = aug[pivot*w+j]; aug[pivot*w+j] = t;
        }
        float sc = aug[col*w+col];
        if ( sc == 0.0f ) { delete[] aug; throw CEMSException( EMS_EXCEPTION ); }
        for ( int j = 0; j < w; j++ ) aug[col*w+j] /= sc;
        for ( int row = 0; row < n; row++ ) {
            if ( row == col ) continue;
            float f = aug[row*w+col];
            for ( int j = 0; j < w; j++ ) aug[row*w+j] -= f * aug[col*w+j];
        }
    }

    for ( int i = 0; i < n; i++ )
        for ( int j = 0; j < n; j++ )
            dst[ i*n + j ] = aug[ i*w + n + j ];
    delete[] aug;
}

static void _gjInvert64f( const double *src, double *dst, int n )
{
    int w = 2 * n;
    double *aug = new (std::nothrow) double[ n * w ];
    if ( !aug ) throw CEMSException( EMS_EXCEPTION );

    for ( int i = 0; i < n; i++ ) {
        for ( int j = 0; j < n; j++ ) aug[ i*w + j ] = src[ i*n + j ];
        for ( int j = n; j < w; j++ ) aug[ i*w + j ] = ( (j-n) == i ) ? 1.0 : 0.0;
    }

    for ( int col = 0; col < n; col++ ) {
        int pivot = col;
        for ( int row = col+1; row < n; row++ )
            if ( fabs(aug[row*w+col]) > fabs(aug[pivot*w+col]) ) pivot = row;
        for ( int j = 0; j < w; j++ ) {
            double t = aug[col*w+j]; aug[col*w+j] = aug[pivot*w+j]; aug[pivot*w+j] = t;
        }
        double sc = aug[col*w+col];
        if ( sc == 0.0 ) { delete[] aug; throw CEMSException( EMS_EXCEPTION ); }
        for ( int j = 0; j < w; j++ ) aug[col*w+j] /= sc;
        for ( int row = 0; row < n; row++ ) {
            if ( row == col ) continue;
            double f = aug[row*w+col];
            for ( int j = 0; j < w; j++ ) aug[row*w+j] -= f * aug[col*w+j];
        }
    }

    for ( int i = 0; i < n; i++ )
        for ( int j = 0; j < n; j++ )
            dst[ i*n + j ] = aug[ i*w + n + j ];
    delete[] aug;
}

// ---------------------------------------------------------------------------

void emssbMatrixInverse( const float *src, float *dst, int n )
{
    _gjInvert32f( src, dst, n );
}

void emssbMatrixInverseD( const double *src, double *dst, int n )
{
    _gjInvert64f( src, dst, n );
}

void emscbMatrixTranspose( const EMSCOMPLEX *src, EMSCOMPLEX *dst, int n, int m )
{
    int k = 0;
    for ( int i = 0; i < n; i++ )
        for ( int j = 0; j < m; j++ )
            dst[ k++ ] = src[ j*n + i ];
}

void emssbMatrixTranspose( const float *src, float *dst, int n, int m )
{
    int k = 0;
    for ( int i = 0; i < n; i++ )
        for ( int j = 0; j < m; j++ )
            dst[ k++ ] = src[ j*n + i ];
}

void emscbMatrixMuliply( const EMSCOMPLEX *src1, int m1, int n1,
                         const EMSCOMPLEX *src2, int m2, int n2,
                         EMSCOMPLEX *dst )
{
    if ( n1 != m2 ) throw CEMSException( EMS_EXCEPTION );

    EMSCOMPLEX *t = new (std::nothrow) EMSCOMPLEX[ m2 * n2 ];
    if ( !t ) throw CEMSException( EMS_EXCEPTION );
    emscbMatrixTranspose( src2, t, n2, m2 );

    int k = 0;
    for ( int i = 0; i < m1*n1; i += n1 )
        for ( int j = 0; j < n2*m2; j += m2 )
            dst[ k++ ] = emscDotProd( &src1[i], &t[j], n1 );
    delete[] t;
}

void emsbMatrixMuliply( const float *src1, int m1, int n1,
                        const float *src2, int m2, int n2,
                        float *dst )
{
    if ( n1 != m2 ) throw CEMSException( EMS_EXCEPTION );

    float *t = new (std::nothrow) float[ m2 * n2 ];
    if ( !t ) throw CEMSException( EMS_EXCEPTION );
    emssbMatrixTranspose( src2, t, n2, m2 );

    int k = 0;
    for ( int i = 0; i < m1*n1; i += n1 )
        for ( int j = 0; j < n2*m2; j += m2 )
            dst[ k++ ] = emssDotProd( &src1[i], &t[j], n1 );
    delete[] t;
}
