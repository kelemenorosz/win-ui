#pragma once

#include <windows.h>
#include <concepts>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <initializer_list>
#include <intrin.h>

namespace rst {

	template <typename T>
	struct point {
		T x;
		T y;
		T z;
		T w;

		point<T>& operator+=(const point<T> other) {
			this->x += other.x;
			this->y += other.y;
			this->z += other.z;
			this->w += other.w;
			return *this;
		}

		friend point<T> operator+(rst::point<T> p, const rst::point<T> other) {
			p += other;
			return p;
		}

		template <typename U>
		point<T>& operator*=(const U scalar) {
			this->x *= scalar;
			this->y *= scalar;
			this->z *= scalar;
			this->w *= scalar;
			return *this;
		}

		template <typename U>
		friend point<T> operator*(rst::point<T> p, const U scalar) {
			p *= scalar;
			return p;
		}

		template <typename U>
		friend point<T> operator*(const U scalar, rst::point<T> p) {
			p *= scalar;
			return p;
		}

		template<typename U>
		point(point<U>& other) {
			this->x = other.x;
			this->y = other.y;
			this->z = other.z;
			this->w = other.w;
		}

	    point(std::initializer_list<T>&& init_list) {
	        this->x = *(init_list.begin() + 0);
	        this->y = *(init_list.begin() + 1);
	        this->z = *(init_list.begin() + 2);
	        this->w = *(init_list.begin() + 3);
	    }

	    point() : x(0), y(0), z(0), w(0) {}
	    point(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

		T get_xyz(int xyz) {
			switch (xyz) {
				case 0:
					return this->x;
					break;
				case 1:
					return this->y;
					break;
				case 2:
					return this->z;
					break;
			}
			throw;
			return 0;
		}

		void normalize3d() {
			T len = sqrt(this->x * this->x + this->y * this->y + this->z * this->z);
			this->x /= len;
			this->y /= len;
			this->z /= len;
		}

	};

	struct color {
		BYTE r;
		BYTE g;
		BYTE b;

		template<std::floating_point T>
		color& operator=(rst::point<T>& other) {
			r = other.x;
			g = other.y;
			b = other.z;
		}

	};

	template <typename T>
	struct rectangle {
		T left;
		T top;
		T right;
		T bottom;
	};

	template <typename T>
	struct bufferlist {
		T* ptr;
		int len;	
	};

	template<typename T>
	struct vertex {

		rst::point<T> position;
		rst::point<T> texture;
		rst::point<T> color;
		rst::point<T> normal;

		vertex() : position(0.0, 0.0, 0.0, 0.0), texture(0.0, 0.0, 0.0, 0.0), color(0.0, 0.0, 0.0, 0.0), normal(0.0, 0.0, 0.0, 0.0) {}
		vertex(T x, T y, T z, T u, T v) : position(x, y, z, 0.0), texture(u, v, 0.0, 0.0), color(0.0, 0.0, 0.0, 0.0), normal(0.0, 0.0, 0.0, 0.0) {}
		vertex(T x, T y, T z, T r, T g, T b) : position(x, y, z, 0.0), color(r, g, b, 0.0), texture(0.0, 0.0, 0.0, 0.0), normal(0.0, 0.0, 0.0, 0.0) {}

		void SetColor(T r, T g, T b) { color = rst::point<T>(r, g, b, 0.0); }

	};

	template<typename T>
	struct plane {
		rst::point<T> normal;
		T d;
	};

	struct homogenous_plane {
		int xyz;
		int sign;
	};

	struct edge_simd_integer {

		int step_x;
		int step_y;

		__m128i diff_x;
		__m128i diff_y;

		__m128i initial;

		edge_simd_integer() = delete;
		edge_simd_integer(rst::point<INT32>& p0, rst::point<INT32>& p1, rst::point<INT>& origin) : step_x(4), step_y(1) {

			INT32 A01 = p0.y - p1.y; 
			INT32 B01 = p1.x - p0.x;
			INT32 C01 = p0.x * p1.y - p0.y * p1.x;

			diff_x = load_si32(A01 * step_x);
			diff_y = load_si32(B01 * step_y);

			__m128i origin_x = load_si32(origin.x + 0, origin.x + 1, origin.x + 2, origin.x + 3);
			__m128i origin_y = load_si32(origin.y);

			__m128i A = load_si32(A01);
			__m128i B = load_si32(B01);
			__m128i C = load_si32(C01);

			A = _mm_mullo_epi32(A, origin_x);
			B = _mm_mullo_epi32(B, origin_y);
			A = _mm_add_epi32(A, B);
			A = _mm_add_epi32(A, C);

			initial = A;

		}

		__m128i get_initial() {
			return initial;
		}

		static __m128i load_si32(INT32 a, INT32 b, INT32 c, INT32 d) {
			INT32 loading_arr[4];
			loading_arr[0] = a;
			loading_arr[1] = b;
			loading_arr[2] = c;
			loading_arr[3] = d;
			return _mm_loadu_si128(reinterpret_cast<__m128i*>(&loading_arr[0]));
		}

		static __m128i load_si32(INT32 a) {
			INT32 loading_arr[4];
			loading_arr[0] = a;
			loading_arr[1] = a;
			loading_arr[2] = a;
			loading_arr[3] = a;
			return _mm_loadu_si128(reinterpret_cast<__m128i*>(&loading_arr[0]));
		}

		static __m128i load_ui32(UINT32 a) {
			UINT32 loading_arr[4];
			loading_arr[0] = a;
			loading_arr[1] = a;
			loading_arr[2] = a;
			loading_arr[3] = a;
			return _mm_loadu_si128(reinterpret_cast<__m128i*>(&loading_arr[0]));
		}

	};

	struct edge_simd {

		__m256d diff_x;
		__m256d diff_y;
		__m256d initial;
		__m256d current;

		edge_simd() = delete;
		edge_simd(const rst::point<double>& p0, const rst::point<double>& p1, const rst::point<double>& origin, int stepX, int stepY) {

			double A01 = p0.y - p1.y; 
			double B01 = p1.x - p0.x;
			double C01 = p0.x * p1.y - p0.y * p1.x;

			diff_x = load_dp(A01 * stepX);
			diff_y = load_dp(B01 * stepY);

			__m256d origin_x = load_dp(origin.x + 0, origin.x + 1, origin.x + 2, origin.x + 3);
			__m256d origin_y = load_dp(origin.y);

			__m256d A = load_dp(A01);
			__m256d B = load_dp(B01);
			__m256d C = load_dp(C01);
				
			A = _mm256_mul_pd(A, origin_x);
			B = _mm256_mul_pd(B, origin_y);
			A = _mm256_add_pd(A, B);
			A = _mm256_add_pd(A, C);

			initial = A;
			current = A;

		}

		void step_x() { current = _mm256_add_pd(current, diff_x); }
		void step_y() { initial = _mm256_add_pd(initial, diff_y); current = initial; }
		__m256d get() { return current; }

		static __m256d load_dp(float a) {
			double loading_arr[4];
			loading_arr[0] = a;
			loading_arr[1] = a;
			loading_arr[2] = a;
			loading_arr[3] = a;
			return _mm256_loadu_pd(&loading_arr[0]);
		}

		static __m256d load_dp(float a, float b, float c, float d) {
			double loading_arr[4];
			loading_arr[0] = a;
			loading_arr[1] = b;
			loading_arr[2] = c;
			loading_arr[3] = d;
			return _mm256_loadu_pd(&loading_arr[0]);
		}

	};

	template <int _Column>
	struct vector;

	template <int _Row, int _Column>
	struct matrix {

		double _matrix[_Row][_Column];

		template<int _Size>
	    static matrix<_Size, _Size> identity() {
	        matrix<_Size, _Size> mat;
	        for (int i = 0; i < _Size; i++) {
	            for (int j = 0; j < _Size; j++) {
	                if (i != j) {
	                    mat._matrix[i][j] = 0;
	                }
	                else {
	                    mat._matrix[i][j] = 1;
	                }
	            }
	        }
	        return mat;
	    }

	    static matrix<4, 4> lookAt(rst::vector<3> up, rst::vector<3> direction, rst::vector<3> position);
	    static matrix<4, 4> rotationX(double angle);
	    static matrix<4, 4> rotationY(double angle);
	    static matrix<4, 4> rotationZ(double angle);
	    static matrix<4, 4> translation(rst::vector<3> vec);

        template <int _Column_Second>
	    friend matrix<_Row, _Column_Second> operator*(const matrix<_Row, _Column> m1, const matrix<_Column, _Column_Second> m2) {
	        matrix<_Row, _Column_Second> mat;
	        for (int i = 0; i < _Row; i++) {
	            for (int j = 0; j < _Column_Second; j++) {

	                mat._matrix[i][j] = 0;
	                for (int k = 0; k < _Column; k++) {
	                    mat._matrix[i][j] += m1._matrix[i][k] * m2._matrix[k][j]; 
	                }
	                
	            }
	        }
	        return mat;
	    }

	    void print() {
	        for (int i = 0; i < _Row; i++) {
	            for (int j = 0; j < _Column; j++) {
	                printf("%f ", this->_matrix[i][j]);
	            }
	            printf("\n");
	        }
	    }

	    template <int _Row_Number, class T, int _Count>
	    void set(T const(&list)[_Count]) {
	        static_assert(_Count == _Column);
	        static_assert(_Row_Number < _Row);
	        for (int i = 0; i < _Column; i++) {
	            this->_matrix[_Row_Number][i] = list[i];
	        }
	    }

        template <int _Row_Number>
	    void set(rst::vector<_Column> v) {
	    	static_assert(_Row_Number < _Row);
	        for (int i = 0; i < _Column; i++) {
	            this->_matrix[_Row_Number][i] = v._vector[i];
	        }
	    }

	    template <int _Column_Number>
	    void set_column(rst::vector<_Row> v) {
	        static_assert(_Column_Number < _Column);
	        for (int i = 0; i < _Row; i++) {
	            this->_matrix[i][_Column_Number] = v._vector[i];
	        }
	    }

	};

	template <int _Column>
	struct vector {

		double _vector[_Column];

	    void print() {
	        for (int i = 0; i < _Column; i++) {
	            printf("%f ", this->_vector[i]);
	        }
	        printf("\n");
	    }

	    template <class T, int _Count>
	    void set(T const(&list)[_Count]) {
	        static_assert(_Count == _Column);
	        for (int i = 0; i < _Column; i++) {
	            this->_vector[i] = list[i];
	        }
	    }

	    template <int _Column_Second>
	    friend vector<_Column_Second> operator*(const vector<_Column> v, const rst::matrix<_Column, _Column_Second> mat) {

	        vector<_Column_Second> v_new;
	        for (int i = 0; i < _Column_Second; i++) {
	            v_new._vector[i] = 0;
	            for (int j = 0; j < _Column; j++) {
	                v_new._vector[i] += v._vector[j] * mat._matrix[j][i];
	            }
	        }
	        return v_new;

	    }

	    vector<_Column>& operator-=(const vector<_Column> other) {
	        for (int i = 0; i < _Column; i++) {
	            this->_vector[i] -= other._vector[i];
	        } 
	        return *this;
	    } 

	    vector<_Column>& operator+=(const vector<_Column> other) {
	    	for (int i = 0; i < _Column; i++) {
	    		this->_vector[i] += other._vector[i];
	    	}
	    	return *this;
	    }

	    vector<_Column>& operator*=(const double scalar) {
	    	for (int i = 0; i < _Column; i++) {
	    		this->_vector[i] *= scalar;
	    	}
	    	return *this;
	    }

	    friend vector<_Column> operator*(vector<_Column> v, const double scalar) {
	    	v *= scalar;
	    	return v;
	    }

	    friend vector<_Column> operator*(const double scalar, vector<_Column> v) {
	    	v *= scalar;
	    	return v;
	    }

	    friend vector<_Column> operator-(vector<_Column> v, const vector<_Column> other) {
	        v -= other;
	        return v;
	    }

		friend vector<_Column> operator+(vector<_Column> v, const vector<_Column> other) {
	        v += other;
	        return v;
	    }
	    


	    void normalize() {

	        double len = 0.0;
	        for (int i = 0; i < _Column; i++) {
	            len += this->_vector[i] * this->_vector[i];
	        }
	        len = sqrt(len);
	        for (int i = 0; i < _Column; i++) {
	            this->_vector[i] = this->_vector[i] / len;
	        }

	        return;
	    }

	    static vector<3> cross(vector<3> v1, vector<3> v2) {

	        vector<3> cross_v;
	        cross_v._vector[0] = v1._vector[1] * v2._vector[2] - v1._vector[2] * v2._vector[1];
	        cross_v._vector[1] = -(v1._vector[0] * v2._vector[2] - v1._vector[2] * v2._vector[0]);
	        cross_v._vector[2] = v1._vector[0] * v2._vector[1] - v1._vector[1] * v2._vector[0];
	        return cross_v;

	    }

	    static double dot(vector<3> v1, vector<3> v2) {

	    	double dot_product = 0.0;
	    	dot_product += v1._vector[0] * v2._vector[0];
			dot_product += v1._vector[1] * v2._vector[1];
	    	dot_product += v1._vector[2] * v2._vector[2];
	    	return dot_product;

	    }

        template <int _Column_New, class T, int _Count>
	    vector<_Column_New> extend(T const(&list)[_Count]) {
	        static_assert(_Count + _Column == _Column_New);
	        vector<_Column_New> v;
	        for (int i = 0; i < _Column; i++) {
	            v._vector[i] = this->_vector[i];
	        }
	        for (int i = _Column; i < _Column_New; i++) {
	            v._vector[i] = list[i - _Column];
	        }
	        return v;
	    }

	    void negate() {
	        for (int i = 0; i < _Column; i++) {
	            this->_vector[i] = -this->_vector[i];
	        }
	    }

	};

	template <std::floating_point T>
	rst::point<int>* trianglelist_to_i(rst::point<T>* trianglelist_f, size_t len, int scale_x, int scale_y);

	template <std::floating_point T, std::signed_integral U>
	rst::point<U>* position_to(rst::vertex<T>* vertexlist, size_t len, int scale_x, int scale_y);

	template <std::floating_point T, std::signed_integral U>
	rst::point<T>* pointlist_to(rst::point<U>* pointlist, size_t len, int scale_x, int scale_y);

	template <typename T>
	T edge(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c);

	template <typename T>
	T sign(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c);

	template <std::signed_integral T>
	bool is_point_in_triangle(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p);

	template <std::signed_integral T>
	bool is_point_in_triangle_verbose(FILE* log_file, rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p);

	template <std::signed_integral T>
	rst::rectangle<T> triangle_bounding_box(rst::point<T> &a, rst::point<T> &b, rst::point<T> &c);

	template <std::floating_point T>
	rst::rectangle<T> triangle_bounding_box(rst::point<T> &a, rst::point<T> &b, rst::point<T> &c);

	template <typename T>
	T min3(T a, T b, T c);
	template <typename T>
	T min2(T a, T b);

	template <typename T>
	T max3(T a, T b, T c);
	template <typename T>
	T max2(T a, T b);

	template <std::signed_integral T>
	bool is_top_left(rst::point<T>& a, rst::point<T>& b);

	template <std::floating_point T>
	bool is_top_left(rst::point<T>& a, rst::point<T>& b);

	// template <std::floating_point T, std::signed_integral U>
	// void normalize(const rst::point<T>& p_f, rst::point<U>& p_i, int scale_x, int scale_y);

	template <std::floating_point T, std::signed_integral U>
	void de_normalize(const rst::point<T>& p_f, rst::point<U>& p_i, int scale_x, int scale_y);

	template <std::floating_point T, std::signed_integral U>
	void de_normalize_tex(const rst::point<T>& p_f, rst::point<U>& p_i, int scale_x, int scale_y);

	template <std::floating_point T>
	void rotate_x(rst::point<T>& p, float angle);
	template <std::floating_point T>
	void rotate_y(rst::point<T>& p, float angle);
	template <std::floating_point T>
	void rotate_z(rst::point<T>& p, float angle);

	template <std::floating_point T>
	void perspective(rst::point<T>& p, float fov, float ratio, float n, float f);

	template <std::floating_point T>
	void perspective_fix(rst::point<T>& p, float fov, float ratio, float n, float f);	

	template <typename T, std::floating_point U>
	rst::point<U> barycentric_coefficients(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p);

	template <std::floating_point T>
	void normalize_3d(rst::point<T>& v);

	template <std::floating_point T>
	T distance_to_plane(rst::plane<T>& plane, rst::point<T>& point);

	template <std::floating_point T>
	bool distance_to_plane(rst::homogenous_plane& plane, rst::point<T>& point);

	// -- A: outside, B: inside
	template <std::floating_point T>
	T signed_distance_ratio(rst::point<T>& a, rst::point<T>& b, rst::homogenous_plane& plane);

	template <std::floating_point T>
	rst::vertex<T> larp(rst::vertex<T>& a, rst::vertex<T>& b, T t);

	template <std::floating_point T>
	rst::point<T> larp(rst::point<T>& a, rst::point<T>& b, T t);

	double radian_to_degree(double angle);
	
	double degree_to_radian(double angle);

};

template <int _Row, int _Column>
rst::matrix<4, 4> rst::matrix<_Row, _Column>::lookAt(rst::vector<3> up, rst::vector<3> target, rst::vector<3> position) {

	rst::vector<3> direction = target - position;
	direction.normalize();

	rst::vector<3> right = rst::vector<1>::cross(up, direction);
	right.normalize();

	up = rst::vector<1>::cross(direction, right);
	up.normalize();

	rst::vector<3> final_position;
	final_position.set({rst::vector<1>::dot(position, right), rst::vector<1>::dot(position, up), rst::vector<1>::dot(position, direction)});

	// rst::matrix<4, 4> axes_mat = rst::matrix<1, 1>::identity<4>();
	// axes_mat.set<0>(right.extend<4>({0.0}));
	// axes_mat.set<1>(up.extend<4>({0.0}));
	// axes_mat.set<2>(direction.extend<4>({0.0}));

	// rst::matrix<4, 4> position_mat = rst::matrix<1, 1>::identity<4>();
	// position.negate();
	// position_mat.set_column<3>(position.extend<4>({1.0}));

	// mat = axes_mat * position_mat;
	// mat = position_mat * axes_mat;

	rst::matrix<4, 4> mat = rst::matrix<1, 1>::identity<4>();
	mat.set_column<0>(right.extend<4>({0.0}));
	mat.set_column<1>(up.extend<4>({0.0}));
	mat.set_column<2>(direction.extend<4>({0.0}));
	// position.negate();
	// final_position.negate();
	mat.set<3>(final_position.extend<4>({1.0}));

	return mat;

}

template <int _Row, int _Column>
rst::matrix<4, 4> rst::matrix<_Row, _Column>::rotationX(double angle) {

	rst::matrix<4, 4> mat = rst::matrix<1, 1>::identity<4>();
	mat.set_column<1>({0.0,  cos(angle), -sin(angle), 0.0});
	mat.set_column<2>({0.0,  sin(angle),  cos(angle), 0.0});

	return mat;

}

template <int _Row, int _Column>
rst::matrix<4, 4> rst::matrix<_Row, _Column>::rotationY(double angle) {

	rst::matrix<4, 4> mat = rst::matrix<1, 1>::identity<4>();
	mat.set_column<0>({  cos(angle), 0.0,  sin(angle), 0.0});
	mat.set_column<2>({ -sin(angle), 0.0,  cos(angle), 0.0});

	return mat;

}

template <int _Row, int _Column>
rst::matrix<4, 4> rst::matrix<_Row, _Column>::rotationZ(double angle) {

	rst::matrix<4, 4> mat = rst::matrix<1, 1>::identity<4>();
	mat.set_column<0>({  cos(angle), -sin(angle), 0.0, 0.0});
	mat.set_column<1>({  sin(angle),  cos(angle), 0.0, 0.0});

	return mat;

}

template <int _Row, int _Column>
rst::matrix<4, 4> rst::matrix<_Row, _Column>::translation(rst::vector<3> vec) {

	rst::matrix<4, 4> mat = rst::matrix<1, 1>::identity<4>();
	mat.set<3>(vec.extend<4>({1.0}));
	return mat;

}

template <std::floating_point T>
rst::point<T> rst::larp(rst::point<T>& a, rst::point<T>& b, T t) {

	rst::point<T> L;
	L.x = a.x + (b.x - a.x) * t;
	L.y = a.y + (b.y - a.y) * t;
	L.z = a.z + (b.z - a.z) * t;
	L.w = a.w + (b.w - a.w) * t;

	return L;

}

template <std::floating_point T>
rst::vertex<T> rst::larp(rst::vertex<T>& a, rst::vertex<T>& b, T t) {

	rst::vertex<T> L;
	L.position = larp(a.position, b.position, t);
	L.texture = larp(a.texture, b.texture, t);
	L.color = larp(a.color, b.color, t);
	L.normal = larp(a.normal, a.normal, t);

	return L;

}

template <std::floating_point T>
T rst::signed_distance_ratio(rst::point<T>& a, rst::point<T>& b, rst::homogenous_plane& plane) {

	// T d1 = (a.get_xyz(plane.xyz) - (a.w * plane.sign) / 2);
	// T d2 = (b.get_xyz(plane.xyz) - (b.w * plane.sign) / 2);
	T d1 = (a.get_xyz(plane.xyz) - (a.w * plane.sign));
	T d2 = (b.get_xyz(plane.xyz) - (b.w * plane.sign));
	T t = d1/(d1-d2);

	return t;

}


template <std::floating_point T>
bool rst::distance_to_plane(rst::homogenous_plane& plane, rst::point<T>& point) {

	T xyz = point.get_xyz(plane.xyz);
	// if ((plane.sign == -1 && xyz <= (-point.w / 2)) || (plane.sign == 1 && xyz >= (point.w / 2))) return false;
	if ((plane.sign == -1 && xyz <= (-point.w)) || (plane.sign == 1 && xyz >= (point.w))) return false;

	return true;

}

template <std::floating_point T>
T rst::distance_to_plane(rst::plane<T>& plane, rst::point<T>& point) {

	return (plane.normal.x * point.x) + (plane.normal.y * point.y) + (plane.normal.z * point.z) + plane.d;

}

template <std::floating_point T>
void rst::normalize_3d(rst::point<T>& v) {

	T len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	v.x /= len;
	v.y /= len;
	v.z /= len;

	return;

}

template <std::floating_point T>
void rst::perspective_fix(rst::point<T>& p, float fov, float ratio, float n, float f) {

	T x = 0.0;
	T y = 0.0;
	T z = 0.0;
	T w = 0.0;
	T A = 0.0;
	T B = 0.0;
	T S = 0.0;
	T top = 0.0;
	T right = 0.0;

	A = -f / (f - n);
	B = -2 * f * n / (f - n);

	S = 1.0/tan(fov * 0.5 * std::numbers::pi / 180);
	top = n * S;
    right = top;

	x = p.x * (n / right);
	y = p.y * (n / top);
	z = p.z * A + B;
	
	if (-p.z > -0.1 && -p.z <= 0.0) {
		w = -0.1;
	}
	else if (-p.z < 0.1 && -p.z >= 0.0) {
		w = 0.1;
	}
	else {
		w = -p.z;
	}

	p.x = x;
	p.y = y;
	p.z = z;
	p.w = w;
	
	return;

}

template <std::floating_point T>
void rst::perspective(rst::point<T>& p, float fov, float ratio, float n, float f) {

	float x = p.x;
	float y = p.y;
	float z = p.z;
	
	float w = -z;
	
	p.x /= w;
	p.y /= w;
	p.z /= w;
	p.w = w;
	
	return;
}

template <typename T>
T rst::edge(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c) {
	return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x); 
}

template <typename T>
T rst::sign(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c) {
	return (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x); 
}


// -- There should be two different implementations of is_point_in_triangle.
// -- One for std::floating_point types and one for std::signed_integral types.
// -- This one works only for signed integrals
template <std::signed_integral T>
bool rst::is_point_in_triangle(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p) {
	int bias0 = is_top_left(a, b) ? 0 : -1;
	int bias1 = is_top_left(b, c) ? 0 : -1;
	int bias2 = is_top_left(c, a) ? 0 : -1;
	if ((edge(a, b, p) + bias0 >= 0) && (edge(b, c, p) + bias1 >= 0) && (edge(c, a, p) + bias2 >= 0)) return true;
	return false;
}

template <std::signed_integral T>
bool rst::is_point_in_triangle_verbose(FILE* log_file, rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p) {
	int bias0 = is_top_left(a, b) ? 0 : -1;
	int bias1 = is_top_left(b, c) ? 0 : -1;
	int bias2 = is_top_left(c, a) ? 0 : -1;
	fprintf(log_file, "edge0: %d, edge1: %d, edge2: %d.\n", edge(b, c, p), edge(c, a, p), edge(a, b, p));
	if ((edge(a, b, p) + bias0 >= 0) && (edge(b, c, p) + bias1 >= 0) && (edge(c, a, p) + bias2 >= 0)) return true;
	return false;
}

template <std::signed_integral T>
rst::rectangle<T> rst::triangle_bounding_box(rst::point<T> &a, rst::point<T> &b, rst::point<T> &c) {

	rst::rectangle<T> rect = {
		min3(a.x, b.x, c.x), min3(a.y, b.y, c.y),
		max3(a.x, b.x, c.x), max3(a.y, b.y, c.y)
	};
	return rect;

}

template <std::floating_point T>
rst::rectangle<T> rst::triangle_bounding_box(rst::point<T> &a, rst::point<T> &b, rst::point<T> &c) {

	rst::rectangle<T> rect = {
		min3(a.x, b.x, c.x), max3(a.y, b.y, c.y),
		max3(a.x, b.x, c.x), min3(a.y, b.y, c.y)
	};
	return rect;

}

template <std::signed_integral T>
bool rst::is_top_left(rst::point<T>& a, rst::point<T>& b) {

	if (a.y == b.y && b.x < a.x) return true; // top rule
	if (a.y > b.y) return true; // left rule
	return false;
	
}

template <std::floating_point T>
bool rst::is_top_left(rst::point<T>& a, rst::point<T>& b) {

	if (a.y == b.y && b.x > a.x) return true; // top rule
	if (a.y < b.y) return true; // left rule
	return false;

}

template <typename T>
T rst::min3(T a, T b, T c) {
	return min2(min2(a, b), c);
}

template <typename T>
T rst::min2(T a, T b) {
	if (a > b) return b;
	return a;
}

template <typename T>
T rst::max3(T a, T b, T c) {
	return max2(max2(a, b), c);
}

template <typename T>
T rst::max2(T a, T b) {
	if (a > b) return a;
	return b;
}

// point_f_t normalize(int x, int y, int scale_x, int scale_y) {
	
// 	point_f_t p = {
// 		(float)x / (float)scale_x,
// 		(float)y / (float)scale_y,
// 		0.0f
// 	};
// 	return p;
	
// }

template <std::floating_point T, std::signed_integral U>
void rst::de_normalize(const rst::point<T>& p_f, rst::point<U>& p_i, int scale_x, int scale_y) {

	p_i.x = (int)((p_f.x + 1.0f) * (float)scale_x);
	p_i.y = (int)((-p_f.y + 1.0f) * (float)scale_y);
	
	return;
	
}

template <std::floating_point T, std::signed_integral U>
void rst::de_normalize_tex(const rst::point<T>& p_f, rst::point<U>& p_i, int scale_x, int scale_y) {

	p_i.x = (int)((p_f.x) * (float)scale_x);
	p_i.y = (int)((p_f.y) * (float)scale_y);
	
	return;
	
}

template <std::floating_point T>
rst::point<int>* rst::trianglelist_to_i(rst::point<T>* trianglelist_f, size_t len, int scale_x, int scale_y) {

	rst::point<int>* trianglelist_i = new rst::point<int>[len];
	for (int i = 0; i < len; ++i) {
		de_normalize(trianglelist_f[i], trianglelist_i[i], scale_x, scale_y);
	}
	
	return trianglelist_i;
	
}

template <std::floating_point T>
void rst::rotate_x(rst::point<T>& p, float angle) {

	float y = p.y;
	float z = p.z;
	
	p.y = cos(angle) * y - sin(angle) * z;
	p.z = sin(angle) * y + cos(angle) * z;
	
	return;
	
}
template <std::floating_point T>
void rst::rotate_y(rst::point<T>& p, float angle) {
	
	float x = p.x;
	float z = p.z;
	
	p.x = cos(angle) * x + sin(angle) * z;
	p.z = -sin(angle) * x + cos(angle) * z;
	
	return;
	
}

template <std::floating_point T>
void rst::rotate_z(rst::point<T>& p, float angle) {
	
	float x = p.x;
	float y = p.y;
	
	p.x = cos(angle) * x - sin(angle) * y;
	p.y = sin(angle) * x + cos(angle) * y;
	
	return;
	
}

// -- Pointlist conversion from floating point to signed integral
template <std::floating_point T, std::signed_integral U>
rst::point<U>* rst::position_to(rst::vertex<T>* vertexlist, size_t len, int scale_x, int scale_y) {

	rst::point<U>* pointlist_si = new rst::point<U>[len];
	for (int i = 0; i < len; ++i) {
		de_normalize(vertexlist[i].position, pointlist_si[i], scale_x, scale_y);
	}
	
	return pointlist_si;

}

// -- Pointlist conversion from signed integral to floating point
template <std::floating_point T, std::signed_integral U>
rst::point<T>* rst::pointlist_to(rst::point<U>* pointlist, size_t len, int scale_x, int scale_y) {
	return nullptr;
}

template <typename T, std::floating_point U>
rst::point<U> rst::barycentric_coefficients(rst::point<T>& a, rst::point<T>& b, rst::point<T>& c, rst::point<T>& p) {

	if (static_cast<double>(edge(a, b, c)) == 0) {

		rst::point<U> barycentric_coefs = {
		1.0,
		0.0,
		0.0,
		0.0};
		return barycentric_coefs;

	}

	rst::point<U> barycentric_coefs = {
		static_cast<double>(edge(b, c, p)) / static_cast<double>(edge(a, b, c)),
		static_cast<double>(edge(c, a, p)) / static_cast<double>(edge(a, b, c)),
		static_cast<double>(edge(a, b, p)) / static_cast<double>(edge(a, b, c)),
		0.0};

	return barycentric_coefs;

}
