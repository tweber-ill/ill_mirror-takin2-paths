/**
 * intersection tests
 * @author Tobias Weber
 * @date 26-apr-2021
 * @note Forked on 26-apr-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license see 'LICENSE' file
 *
 * References:
 *  * http://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/index.html
 *  * https://www.boost.org/doc/libs/1_76_0/libs/geometry/doc/html/geometry/reference/algorithms/buffer/buffer_7_with_strategies.html
 *  * https://github.com/boostorg/geometry/tree/develop/example
 *  * https://www.boost.org/doc/libs/1_76_0/libs/test/doc/html/index.html
 */

#define BOOST_TEST_MODULE test_intersections_circle_polys

#include <iostream>
#include <fstream>
#include <tuple>

#include <boost/geometry.hpp>
#include <boost/test/included/unit_test.hpp>
#include <boost/type_index.hpp>
namespace bgeo = boost::geometry;
namespace strat = bgeo::strategy::buffer;
namespace test = boost::unit_test;
namespace ty = boost::typeindex;

#include "src/libs/lines.h"

template<class T> using t_vec = tl2::vec<T, std::vector>;
template<class T> using t_vertex = bgeo::model::point<T, 2, bgeo::cs::cartesian>;
template<class T> using t_poly = bgeo::model::polygon<t_vertex<T>, true /*cw*/, false /*closed*/>;
template<class T> using t_polys = bgeo::model::multi_polygon<t_poly<T>, std::vector>;
template<class T> using t_box = bgeo::model::box<t_vertex<T>>;


BOOST_AUTO_TEST_CASE_TEMPLATE(test_intersections_circle_polys, t_real, 
	decltype(std::tuple</*float,*/ double, long double>{}))
{
	std::cout << "Testing with " << ty::type_id_with_cvr<t_real>().pretty_name() 
		<< " type." << std::endl;

	constexpr const std::size_t NUM_TESTS = 1000;
	const t_real eps = std::sqrt(std::numeric_limits<t_real>::epsilon());
	const t_real cmp_eps = 1e-2;

	t_real rad_min = 0.5;
	t_real rad_max = 5.;
	t_real x_min = -5.;
	t_real x_max = 5.;
	t_real y_min = -5.;
	t_real y_max = 5.;

	for(std::size_t i=0; i<NUM_TESTS; ++i)
	{
		t_real rad1 = tl2::get_rand<t_real>(rad_min, rad_max);
		t_real x1 = tl2::get_rand<t_real>(x_min, x_max);
		t_real y1 = tl2::get_rand<t_real>(y_min, y_max);

		t_real box_x1 = tl2::get_rand<t_real>(x_min, x_max);
		t_real box_x2 = tl2::get_rand<t_real>(x_min, x_max);
		t_real box_y1 = tl2::get_rand<t_real>(y_min, y_max);
		t_real box_y2 = tl2::get_rand<t_real>(y_min, y_max);

		if(box_x2 < box_x1)
			std::swap(box_x1, box_x2);
		if(box_y2 < box_y1)
			std::swap(box_y1, box_y2);


		// circle
		t_polys<t_real> circle1;
		bgeo::buffer(t_vertex<t_real>{x1, y1},
			circle1,
			strat::distance_symmetric<t_real>{rad1},
			strat::side_straight{},
			strat::join_round{128},
			strat::end_round{128},
			strat::point_circle{512});

		// polygon
		t_box<t_real> poly1;
		poly1.min_corner() = t_vertex<t_real>{box_x1, box_y1};
		poly1.max_corner() = t_vertex<t_real>{box_x2, box_y2};

		// intersections
		std::vector<t_vertex<t_real>> inters_circle_poly;
		bgeo::intersection(circle1, poly1, inters_circle_poly);

		std::sort(inters_circle_poly.begin(), inters_circle_poly.end(),
			[cmp_eps](const auto& vert1, const auto& vert2) -> bool
			{
				if(tl2::equals<t_real>(bgeo::get<0>(vert1), bgeo::get<0>(vert2), cmp_eps))
					return bgeo::get<1>(vert1) < bgeo::get<1>(vert2);
				return bgeo::get<0>(vert1) < bgeo::get<0>(vert2);
			});


		// custom intersection calculation
		std::vector<t_vec<t_real>> poly;
		poly.emplace_back(tl2::create<t_vec<t_real>>({box_x1, box_y1}));
		poly.emplace_back(tl2::create<t_vec<t_real>>({box_x2, box_y1}));
		poly.emplace_back(tl2::create<t_vec<t_real>>({box_x2, box_y2}));
		poly.emplace_back(tl2::create<t_vec<t_real>>({box_x1, box_y2}));

		auto custom_inters = geo::intersect_circle_polylines<t_vec<t_real>>(
			tl2::create<t_vec<t_real>>({x1, y1}), rad1, poly, true);

		std::sort(custom_inters.begin(), custom_inters.end(),
			[cmp_eps](const auto& vec1, const auto& vec2) -> bool
			{
				if(tl2::equals<t_real>(vec1[0], vec2[0], cmp_eps))
					return vec1[1] < vec2[1];
				return vec1[0] < vec2[0];
			});

		// collision calculation
		bool collide = geo::collide_circle_poly<t_vec<t_real>>(
			tl2::create<t_vec<t_real>>({x1, y1}), rad1, poly);


		auto print_objs = [&x1, &y1, &rad1, &box_x1, &box_x2, &box_y1, &box_y2, 
			&custom_inters, &inters_circle_poly]()
		{
			std::cout << "--------------------------------------------------------------------------------" << std::endl;
			std::cout << "circle: mid = (" << x1 << ", " << y1 << ")"
				<< ", rad = " << rad1 << std::endl;
			std::cout << "box: min = (" << box_x1 << ", " << box_y1 << ")"
				<< ", max = (" << box_x2 << ", " << box_y2 << ")" << std::endl;
			std::cout << std::endl;

			std::cout << "custom circle-poly intersection points:" << std::endl;
			for(const t_vec<t_real>& pt : custom_inters)
			{
				using namespace tl2_ops;
				std::cout << "\t" << pt << std::endl;
			}
			std::cout << std::endl;

			std::cout << "boost::geo circle-poly intersection points:" << std::endl;
			for(const auto& vert : inters_circle_poly)
			{
				std::cout << "\t" << bgeo::get<0>(vert) << 
					"; " << bgeo::get<1>(vert) << std::endl;
			}
			std::cout << "--------------------------------------------------------------------------------" << std::endl;
			std::cout << std::endl;
		};


		if(custom_inters.size() > 0)
			BOOST_TEST((collide));


		bool sizes_equal = (inters_circle_poly.size() == custom_inters.size());
		BOOST_TEST((sizes_equal));
		if(!sizes_equal)
		{
			print_objs();
			continue;
		}


		for(std::size_t idx=0; idx<custom_inters.size(); ++idx)
		{
			const t_vec<t_real>& inters1 = custom_inters[idx];
			const t_vertex<t_real>& inters2 = inters_circle_poly[idx];

			bool equals_x = tl2::equals<t_real>(inters1[0], bgeo::get<0>(inters2), cmp_eps);
			bool equals_y = tl2::equals<t_real>(inters1[1], bgeo::get<1>(inters2), cmp_eps);

			BOOST_TEST((equals_x && equals_y));
			if(!equals_x || !equals_y)
				print_objs();
		}
	}
}
