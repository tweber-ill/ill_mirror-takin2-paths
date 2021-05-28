/**
 * image processing
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @note Forked on 26-may-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 * @license see 'LICENSE' file
 * 
 * @references:
 *   - https://www.boost.org/doc/libs/1_69_0/libs/gil/doc/html/tutorial.html
 */

#include <cstdlib>
#include <concepts>

#include "tlibs2/libs/maths.h"


namespace geo {

/**
 * requirements for a gil image view
 */
template<class t_imageview>
concept is_imageview = requires(const t_imageview& img, std::size_t y, std::size_t x)
{
	x = img.width();
	y = img.height();

	img.row_begin(y)[x];
};


/**
 * get a pixel from an image view
 */
template<class t_imageview> requires is_imageview<t_imageview>
typename gil::channel_type<t_imageview>::type 
get_pixel(const t_imageview& img, int x, int y)
{
	using t_pixel = typename gil::channel_type<t_imageview>::type;

	if(x >= img.width() || y >= img.height() || x<0 || y<0)
		return t_pixel{};

	return img.row_begin(y)[x];
}


/**
 * set a pixel in an image view
 */
template<class t_imageview> requires is_imageview<t_imageview>
void set_pixel(t_imageview& img, int x, int y, 
	typename gil::channel_type<t_imageview>::type pixel)
{
	if(x >= img.width() || y >= img.height() || x<0 || y<0)
		return;

	img.row_begin(y)[x] = pixel;
}


/**
 * get the width and height from an image view
 */
template<class t_imageview> requires is_imageview<t_imageview>
std::pair<std::size_t, std::size_t> get_image_dims(const t_imageview& img)
{
	return std::make_pair(img.width(), img.height());
}


/**
 * boundary tracing
 * @see http://www.imageprocessingplace.com/downloads_V3/root_downloads/tutorials/contour_tracing_Abeer_George_Ghuneim/ray.html
 */
template<class t_vec, 
	class t_imageview, class t_boundaryview>
requires tl2::is_vec<t_vec>
std::vector<std::vector<t_vec>> trace_boundary(
	const t_imageview& img, t_boundaryview& boundary)
{
	// contour polygons
	std::vector<std::vector<t_vec>> contours;
	std::vector<t_vec> contour;


	// contour already seen?
	auto already_seen = [&contours](const t_vec& vec) -> bool
	{
		for(const auto& contour : contours)
		{
			if(std::find(contour.begin(), contour.end(), vec) != contour.end())
				return true;
		}

		return false;
	};


	// next possible position depending on direction
	auto get_next_dir = [](const t_vec& dir, t_vec& next_dir, int iter=0) -> bool
	{
		const t_vec next_dirs[] = 
		{
			tl2::create<t_vec>({ -1, -1 }), // 0
			tl2::create<t_vec>({  0, -1 }), // 1
			tl2::create<t_vec>({  1, -1 }), // 2
			tl2::create<t_vec>({  1,  0 }), // 3
			tl2::create<t_vec>({  1,  1 }), // 4
			tl2::create<t_vec>({  0,  1 }), // 5
			tl2::create<t_vec>({ -1,  1 }), // 6
			tl2::create<t_vec>({ -1,  0 }), // 7
		};
		
		const t_vec back_dir = -dir;
		std::size_t idx = 0;
		bool has_next_dir = false;

		for(int i=0; i<8; ++i)
		{
			if(back_dir == next_dirs[i])
			{
				idx = (iter + i+1) % 8;
				has_next_dir = true;
				break;
			}
		}

		if(!has_next_dir)
			return false;

		next_dir = next_dirs[idx];
		return true;
	};


	// find multiple contours
	t_vec start = tl2::create<t_vec>({0, 0});

	while(true)
	{
		// find start pixel
		bool start_found = 0;
		auto [width, height] = get_image_dims(img);

		for(int y=start[1]; y<(int)height; ++y)
		{
			for(int x=0; x<(int)width; ++x)
			{
				if(get_pixel(img, x, y))
				{
					// for multiple contours: inside a contour
					if(get_pixel(img, x-1, y))
						continue;

					t_vec vec = tl2::create<t_vec>({x, y});

					if(already_seen(vec))
						continue;

					start = vec;
					start_found = 1;
					break;
				}
			}

			if(start_found)
				break;
		}

		if(!start_found)
			return contours;

		contour.push_back(start);
		set_pixel(boundary, start[0], start[1], 0xff);


		// trace boundary
		t_vec pos = start;
		t_vec dir = tl2::create<t_vec>({1, 0});
		t_vec next_dir = tl2::create<t_vec>({0, 0});


		while(1)
		{
			bool has_next_dir = false;

			for(int i=0; i<8; ++i)
			{
				if(get_next_dir(dir, next_dir, i) && get_pixel(img, pos[0]+next_dir[0], pos[1]+next_dir[1]))
				{
					has_next_dir = true;
					break;
				}
			}

			if(has_next_dir)
			{
				dir[0] = next_dir[0];
				dir[1] = next_dir[1];

				pos[0] += dir[0];
				pos[1] += dir[1];

				contour.push_back(pos);
				set_pixel(boundary, pos[0], pos[1], 0xff);
			}
			else
			{
				break;
			}

			// back at start
			if(pos[0] == start[0] && pos[1] == start[1])
				break;
		}

		contours.emplace_back(std::move(contour));
	}

	return contours;
}
}
