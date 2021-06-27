/**
 * image processing concepts, containers and algorithms
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2021
 * @note Forked on 26-may-2021 from my privately developed "misc" project (https://github.com/t-weber/misc).
 * @license see 'LICENSE' file
 *
 * @references:
 *   - https://www.boost.org/doc/libs/1_69_0/libs/gil/doc/html/tutorial.html
 */

#include <concepts>
#include <cstdlib>
#include <boost/gil/image.hpp>

#include "tlibs2/libs/maths.h"


namespace geo {

// ----------------------------------------------------------------------------
// concepts
// ----------------------------------------------------------------------------
/**
 * requirements for the Image class
 */
template<class t_image>
concept is_image = requires(t_image& img, std::size_t y, std::size_t x)
{
	typename t_image::value_type;

	x = img.GetWidth();
	y = img.GetHeight();

	img.SetPixel(x, y, 0);
	img.GetPixel(x, y);
};


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
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// classes
// ----------------------------------------------------------------------------
/**
 * simple image class
 */
template<class t_pixel = bool>
class Image
{
public:
	using value_type = t_pixel;

public:
	Image() = default;
	~Image() = default;


	Image(std::size_t w, std::size_t h)
	{
		Init(w, h);
	}


	Image(const Image<t_pixel>& img)
		: Image(img.GetWidth(), img.GetHeight())
	{
		SetImage(img.m_img.get());
	}


	void Init(std::size_t w, std::size_t h)
	{
		m_width = w;
		m_height = h;
		m_img = std::make_unique<t_pixel[]>(w*h);
	}


	void Clear()
	{
		m_width = 0;
		m_height = 0;
		m_img.reset();
	}


	std::size_t GetWidth() const
	{
		return m_width;
	}


	std::size_t GetHeight() const
	{
		return m_height;
	}


	t_pixel GetPixel(std::size_t x, std::size_t y) const
	{
		if(x < GetWidth() && y < GetHeight())
			return m_img[y*m_width + x];
		return t_pixel{};
	}


	void SetPixel(std::size_t x, std::size_t y, t_pixel pix)
	{
		if(x < GetWidth() && y < GetHeight())
			m_img[y*m_width + x] = pix;
	}


	void SetImage(const t_pixel* img)
	{
		for(std::size_t y = 0; y < GetHeight(); ++y)
			for(std::size_t x = 0; x < GetWidth(); ++x)
				SetPixel(x, y, img[y*GetWidth() + x]);
	}


private:
	std::size_t m_width{}, m_height{};
	std::unique_ptr<t_pixel[]> m_img;
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// interface wrapper functions
// ----------------------------------------------------------------------------
/**
 * get a pixel from an image
 */
template<class t_image> requires is_image<t_image>
typename t_image::value_type 
get_pixel(const t_image& img, int x, int y)
{
	using t_pixel = typename t_image::value_type;

	if(x >= (int)img.GetWidth() || y >= (int)img.GetHeight() || x<0 || y<0)
		return t_pixel{};

	return img.GetPixel(x, y);
}


/**
 * set a pixel in an image
 */
template<class t_image> requires is_image<t_image>
void set_pixel(t_image& img, int x, int y, 
	typename t_image::value_type pixel)
{
	if(x >= (int)img.GetWidth() || y >= (int)img.GetHeight() || x<0 || y<0)
		return;

	img.SetPixel(x, y, pixel);
}


/**
 * get the width and height from an image
 */
template<class t_image> requires is_image<t_image>
std::pair<std::size_t, std::size_t> get_image_dims(const t_image& img)
{
	return std::make_pair(img.GetWidth(), img.GetHeight());
}


/**
 * get a pixel from an image view
 */
template<class t_imageview> requires is_imageview<t_imageview>
typename boost::gil::channel_type<t_imageview>::type 
get_pixel(const t_imageview& img, int x, int y)
{
	using t_pixel = typename boost::gil::channel_type<t_imageview>::type;

	if(x >= img.width() || y >= img.height() || x<0 || y<0)
		return t_pixel{};

	return img.row_begin(y)[x];
}


/**
 * set a pixel in an image view
 */
template<class t_imageview> requires is_imageview<t_imageview>
void set_pixel(t_imageview& img, int x, int y, 
	typename boost::gil::channel_type<t_imageview>::type pixel)
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
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// algorithms
// ----------------------------------------------------------------------------
/**
 * boundary tracing
 * @see http://www.imageprocessingplace.com/downloads_V3/root_downloads/tutorials/contour_tracing_Abeer_George_Ghuneim/ray.html
 */
template<class t_vec,
	class t_imageview, class t_boundaryview = t_imageview>
requires tl2::is_vec<t_vec>
std::vector<std::vector<t_vec>> trace_boundary(
	const t_imageview& img, t_boundaryview* boundary = nullptr)
{
	// contour polygons
	std::vector<std::vector<t_vec>> contours;


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
		static const t_vec next_dirs[] =
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
		std::vector<t_vec> contour;

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

		//contour.push_back(start);
		if(boundary)
			set_pixel<t_boundaryview>(*boundary, start[0], start[1], 0xff);


		// trace boundary
		t_vec pos = start;
		t_vec dir = tl2::create<t_vec>({1, 0});
		t_vec next_dir = tl2::create<t_vec>({0, 0});

		while(true)
		{
			bool has_next_dir = false;

			for(int i=0; i<8; ++i)
			{
				if(get_next_dir(dir, next_dir, i)
					&& get_pixel(img, pos[0]+next_dir[0], pos[1]+next_dir[1]))
				{
					has_next_dir = true;
					break;
				}
			}

			if(has_next_dir)
			{
				dir[0] = next_dir[0];
				dir[1] = next_dir[1];

				// don't insert the same point multiple times
				if(!tl2::equals_0<t_vec>(dir))
				{
					pos[0] += dir[0];
					pos[1] += dir[1];

					contour.push_back(pos);
					if(boundary)
						set_pixel<t_boundaryview>(*boundary, pos[0], pos[1], 0xff);
				}
			}
			else
			{
				break;
			}

			// back at start
			if(pos[0] == start[0] && pos[1] == start[1])
				break;
		}

		if(contour.size())
			contours.emplace_back(std::move(contour));
		else
			break;
	}

	return contours;
}
// ----------------------------------------------------------------------------

} // geo
