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


namespace geo {


/**
 * get a pixel from an image view
 */
template<class t_imageview>
typename gil::channel_type<t_imageview>::type 
get_pixel(const t_imageview& img, int x, int y)
{
	using t_pixel = typename gil::channel_type<t_imageview>::type;

	if(x >= img.width() || y >= img.height())
		return t_pixel{};

	return img.row_begin(y)[x];
}


/**
 * set a pixel in an image view
 */
template<class t_imageview>
void set_pixel(t_imageview& img, int x, int y, typename gil::channel_type<t_imageview>::type pixel)
{
	if(x >= img.width() || y >= img.height())
		return;

	img.row_begin(y)[x] = pixel;
}


/**
 * boundary tracing
 * @see http://www.imageprocessingplace.com/downloads_V3/root_downloads/tutorials/contour_tracing_Abeer_George_Ghuneim/ray.html
 */
template<class t_imageview, class t_boundaryview>
void trace_boundary(const t_imageview& img, t_boundaryview& boundary)
{
	// find start pixel
	int start[2] = {0, 0};
	bool start_found = 0;

	for(int y=0; y<img.height(); ++y)
	{
		for(int x=0; x<img.width(); ++x)
		{
			if(get_pixel(img, x, y))
			{
				start[0] = x;
				start[1] = y;
				start_found = 1;
				break;
			}

			if(start_found)
				break;
		}
	}

	if(!start_found)
		return;
	else
		set_pixel(boundary, start[0], start[1], 0xff);


	// trace boundary
	int pos[2] = { start[0], start[1] };
	int dir[2] = {1, 0};
	int next_dir[2] = {0, 0};


	// next possible position depending on direction
	auto get_next_dir = [](const int* dir, int* next_dir, int iter=0) -> bool
	{
		const int next_dirs[][2] = 
		{
			{ -1, -1 }, // 0
			{  0, -1 }, // 1
			{  1, -1 }, // 2
			{  1,  0 }, // 3
			{  1,  1 }, // 4
			{  0,  1 }, // 5
			{ -1,  1 }, // 6
			{ -1,  0 }, // 7
		};
		
		const int back_dir[2] = { -dir[0], -dir[1] };
		std::size_t idx = 0;
		bool has_next_dir = false;

		for(int i=0; i<8; ++i)
		{
			if(back_dir[0] == next_dirs[i][0] && back_dir[1] == next_dirs[i][1])
			{
				idx = (iter + i+1) % 8;
				has_next_dir = true;
				break;
			}
		}

		if(!has_next_dir)
			return false;

		next_dir[0] = next_dirs[idx][0];
		next_dir[1] = next_dirs[idx][1];
		return true;
	};


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
}
}
