/**
 * \file PoissonGenerator.h
 * \brief
 *
 * Poisson Disk Points Generator
 *
 * \version 1.1.5
 * \date 16/06/2019
 * \author Sergey Kosarevsky, 2014-2019
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */

/*
	Usage example:

		#define POISSON_PROGRESS_INDICATOR 1
		#include "PoissonGenerator.h"
		...
		PoissonGenerator::DefaultPRNG PRNG;
		const auto Points = PoissonGenerator::GeneratePoissonPoints( NumPoints, PRNG );
*/

// Fast Poisson Disk Sampling in Arbitrary Dimensions
// http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf

// Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/

/* Versions history:
 *    1.1.5    Jun 16, 2019      In-class initializers, default ctors, naming, shorter code
 *		1.1.4 	Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header file, disabled by default
 *		1.1.3a	Jun  9, 2016		update constructor for DefaultPRNG
 *		1.1.3		Mar 10, 2016		Header-only library, no global mutable state
 *		1.1.2		Apr  9, 2015		Output a text file with XY coordinates
 *		1.1.1		May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
 *    1.1		May  7, 2014		Support of density maps
 *		1.0		May  6, 2014
*/
#ifndef PoissonGenerator_H_
#define PoissonGenerator_H_

#include <vector>
#include <random>
#include <stdint.h>
#include <time.h>
#include <iostream>

namespace PoissonGenerator
{

static const char* Version = "1.1.5 (16/06/2019)";

class DefaultPRNG
{
public:
	DefaultPRNG()
	: gen_( std::random_device()() )
	, dis_( 0.0f, 1.0f )
	{
		// prepare PRNG
		gen_.seed( time( nullptr ) );
	}

	explicit DefaultPRNG( uint32_t seed )
	: gen_( seed )
	, dis_( 0.0f, 1.0f )
	{
	}

	float randomFloat()
	{
		return static_cast<float>( dis_( gen_ ) );
	}

	int randomInt( int maxValue )
	{
		std::uniform_int_distribution<> disInt( 0, maxValue );
		return disInt( gen_ );
	}

private:
	std::mt19937 gen_;
	std::uniform_real_distribution<float> dis_;
};

static struct Point
{
	Point() = default;
	Point( float X, float Y )
	: x( X )
	, y( Y )
	, valid_( true )
	{}
	float x = 0.0f;
	float y = 0.0f;
	bool valid_ = false;
	//
	bool isInRectangle() const
	{
		return x >= 0 && y >= 0 && x <= 1 && y <= 1;
	}
	//
	bool isInCircle() const
	{
		const float fx = x - 0.5f;
		const float fy = y - 0.5f;
		return ( fx*fx + fy*fy ) <= 0.25f;
	}
};

static struct GridPoint
{
	GridPoint() = delete;
	GridPoint( int X, int Y )
	: x( X )
	, y( Y )
	{}
	int x;
	int y;
};

inline float getDistance( const Point& P1, const Point& P2 )
{
	return sqrt( ( P1.x - P2.x ) * ( P1.x - P2.x ) + ( P1.y - P2.y ) * ( P1.y - P2.y ) );
}

inline GridPoint imageToGrid( const Point& P, float cellSize )
{
	return GridPoint( ( int )( P.x / cellSize ), ( int )( P.y / cellSize ) );
}

struct Grid
{
	Grid( int w, int h, float cellSize )
	: w_( w )
	, h_( h )
	, cellSize_( cellSize )
	{
		grid_.resize( h_ );
		for ( auto i = grid_.begin(); i != grid_.end(); i++ ) { i->resize( w ); }
	}
	void insert( const Point& p )
	{
		const GridPoint g = imageToGrid( p, cellSize_ );
		grid_[ g.x ][ g.y ] = p;
	}
	bool isInNeighbourhood( const Point& point, float minDist, float cellSize )
	{
		GridPoint g = imageToGrid( point, cellSize );

		// number of adjucent cells to look for neighbour points
		const int D = 5;

		// scan the neighbourhood of the point in the grid
		for ( int i = g.x - D; i < g.x + D; i++ )
		{
			for ( int j = g.y - D; j < g.y + D; j++ )
			{
				if ( i >= 0 && i < w_ && j >= 0 && j < h_ )
				{
					const Point P = grid_[ i ][ j ];

					if ( P.valid_ && getDistance( P, point ) < minDist ) { return true; }
				}
			}
		}

		return false;
	}

private:
	int w_;
	int h_;
	float cellSize_;
	std::vector< std::vector<Point> > grid_;
};

template <typename PRNG>
inline Point popRandom( std::vector<Point>& points, PRNG& generator )
{
	const int idx = generator.randomInt( points.size()-1 );
	const Point p = points[ idx ];
	points.erase( points.begin() + idx );
	return p;
}

template <typename PRNG>
inline Point generateRandomPointAround( const Point& p, float minDist, PRNG& generator )
{
	// start with non-uniform distribution
	const float R1 = generator.randomFloat();
	const float R2 = generator.randomFloat();

	// radius should be between MinDist and 2 * MinDist
	const float radius = minDist * ( R1 + 1.0f );

	// random angle
	const float angle = 2 * 3.141592653589f * R2;

	// the new point is generated around the point (x, y)
	const float x = p.x + radius * cos( angle );
	const float y = p.y + radius * sin( angle );

	return Point( x, y );
}

/**
	Return a vector of generated points

	NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
	Circle  - 'true' to fill a circle, 'false' to fill a rectangle
	MinDist - minimal distance estimator, use negative value for default
**/
template <typename PRNG = DefaultPRNG>
inline std::vector<Point> generatePoissonPoints(
	size_t numPoints,
	PRNG& generator,
	int newPointsCount = 30,
	bool isCircle = true,
	float minDist = -1.0f
)
{
	if ( minDist < 0.0f )
	{
		minDist = sqrt( float(numPoints) ) / float(numPoints);
	}

	std::vector<Point> samplePoints;
	std::vector<Point> processList;

	// create the grid
	const float cellSize = minDist / sqrt( 2.0f );

	const int gridW = ( int )ceil( 1.0f / cellSize );
	const int gridH = ( int )ceil( 1.0f / cellSize );

	Grid grid( gridW, gridH, cellSize );

	Point firstPoint;
 	do {
		firstPoint = Point( generator.randomFloat(), generator.randomFloat() );	     
	} while (!(isCircle ? firstPoint.isInCircle() : firstPoint.isInRectangle()));

	// update containers
	processList.push_back( firstPoint );
	samplePoints.push_back( firstPoint );
	grid.insert( firstPoint );

	// generate new points for each point in the queue
	while ( !processList.empty() && samplePoints.size() < numPoints )
	{
#if POISSON_PROGRESS_INDICATOR
		// a progress indicator, kind of
		if ( samplePoints.size() % 100 == 0 ) std::cout << ".";
#endif // POISSON_PROGRESS_INDICATOR

		const Point point = popRandom<PRNG>( processList, generator );

		for ( int i = 0; i < newPointsCount; i++ )
		{
			const Point newPoint = generateRandomPointAround( point, minDist, generator );
			const bool canFitPoint = isCircle ? newPoint.isInCircle() : newPoint.isInRectangle();

			if ( canFitPoint && !grid.isInNeighbourhood( newPoint, minDist, cellSize ) )
			{
				processList.push_back( newPoint );
				samplePoints.push_back( newPoint );
				grid.insert( newPoint );
				continue;
			}
		}
	}

#if POISSON_PROGRESS_INDICATOR
	std::cout << std::endl << std::endl;
#endif // POISSON_PROGRESS_INDICATOR

	return samplePoints;
}

} // namespace PoissonGenerator

#endif