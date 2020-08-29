#include <yafraycore/triangle.h>


#ifdef __clang__
#define inline  // aka inline removal
#endif

BEGIN_YAFRAY

int triBoxClip__(const double b_min[3], const double b_max[3], const double triverts[3][3], Bound &box, void *n_dat);
int triPlaneClip__(double pos, int axis, bool lower, Bound &box, void *o_dat, void *n_dat);

void Triangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = getNormal();

	float u = data.b_0_, v = data.b_1_, w = data.b_2_;

	if(mesh_->is_smooth_ || mesh_->normals_exported_)
	{
		Vec3 va = (na_ >= 0) ? mesh_->getVertexNormal(na_) : sp.ng_;
		Vec3 vb = (nb_ >= 0) ? mesh_->getVertexNormal(nb_) : sp.ng_;
		Vec3 vc = (nc_ >= 0) ? mesh_->getVertexNormal(nc_) : sp.ng_;

		sp.n_ = u * va + v * vb + w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;

	if(mesh_->has_orco_)
	{
		// if the object is an instance, the vertex positions are the orcos
		Point3 const &p_0 = mesh_->getVertex(pa_ + 1);
		Point3 const &p_1 = mesh_->getVertex(pb_ + 1);
		Point3 const &p_2 = mesh_->getVertex(pc_ + 1);

		sp.orco_p_ = u * p_0 + v * p_1 + w * p_2;

		sp.orco_ng_ = ((p_1 - p_0) ^ (p_2 - p_0)).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.has_orco_ = false;
		sp.orco_ng_ = sp.ng_;
	}

	Point3 const &p_0 = mesh_->getVertex(pa_);
	Point3 const &p_1 = mesh_->getVertex(pb_);
	Point3 const &p_2 = mesh_->getVertex(pc_);

	if(mesh_->has_uv_)
	{
		size_t uvi = self_index_ * 3;
		const Uv &uv_1 = mesh_->uv_values_[mesh_->uv_offsets_[uvi]];
		const Uv &uv_2 = mesh_->uv_values_[mesh_->uv_offsets_[uvi + 1]];
		const Uv &uv_3 = mesh_->uv_values_[mesh_->uv_offsets_[uvi + 2]];

		//eh...u, v and w are actually the barycentric coords, not some UVs...quite annoying, i know...
		sp.u_ = u * uv_1.u_ + v * uv_2.u_ + w * uv_3.u_;
		sp.v_ = u * uv_1.v_ + v * uv_2.v_ + w * uv_3.v_;

		// calculate dPdU and dPdV
		float du_1 = uv_1.u_ - uv_3.u_;
		float du_2 = uv_2.u_ - uv_3.u_;
		float dv_1 = uv_1.v_ - uv_3.v_;
		float dv_2 = uv_2.v_ - uv_3.v_;
		float invdet, det = du_1 * dv_2 - dv_1 * du_2;

		if(std::fabs(det) > 1e-30f)
		{
			invdet = 1.f / det;
			Vec3 dp_1 = p_0 - p_2;
			Vec3 dp_2 = p_1 - p_2;
			sp.dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp.dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
		}
		else
		{
			// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => U = u, V = v; (arbitrary choice)
			sp.dp_du_ = p_1 - p_0;
			sp.dp_dv_ = p_2 - p_1;
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => U = u, V = v; (arbitrary choice)
		sp.dp_du_ = p_1 - p_0;
		sp.dp_dv_ = p_2 - p_1;
		sp.u_ = 0.f;  //fix for nan black dots using glossy with bump mapping without UV map associated. Forcing initialization to avoid non-initialized values in that case.
		sp.v_ = 0.f;  //fix for nan black dots using glossy with bump mapping without UV map associated. Forcing initialization to avoid non-initialized values in that case.
	}

	//Copy original dPdU and dPdV before normalization to the "absolute" dPdU and dPdV (for mipmap calculations)
	sp.dp_du_abs_ = sp.dp_du_;
	sp.dp_dv_abs_ = sp.dp_dv_;

	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();

	sp.object_ = mesh_;
	sp.prim_num_ = self_index_;
	sp.material_ = material_;
	sp.p_ = hit;
	createCs__(sp.n_, sp.nu_, sp.nv_);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y_ = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z_ = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x_ = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y_ = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.n_ * sp.dp_dv_;
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool Triangle::clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		bool lower = axis & ~3;
		int axis_calc = axis & 3;
		double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		int res = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(res > 1) goto WHOOPS;
		return (res == 0) ? true : false;
	}
	else // initial clip
	{
WHOOPS:

		double t_points[3][3];

		Point3 const &a = mesh_->getVertex(pa_);
		Point3 const &b = mesh_->getVertex(pb_);
		Point3 const &c = mesh_->getVertex(pc_);

		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		int res = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (res == 0) ? true : false;
	}
	return true;
}

float Triangle::surfaceArea() const
{
	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	Vec3 edge_1, edge_2;
	edge_1 = b - a;
	edge_2 = c - a;
	return 0.5 * (edge_1 ^ edge_2).length();
}

void Triangle::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	Point3 const &a = mesh_->getVertex(pa_);
	Point3 const &b = mesh_->getVertex(pb_);
	Point3 const &c = mesh_->getVertex(pc_);

	float su_1 = fSqrt__(s_1);
	float u = 1.f - su_1;
	float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = getNormal();
}

// triangleInstance_t Methods

void TriangleInstance::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = getNormal();
	int pa = m_base_->pa_;
	int pb = m_base_->pb_;
	int pc = m_base_->pc_;
	int na = m_base_->na_;
	int nb = m_base_->nb_;
	int nc = m_base_->nc_;

	size_t self_index = m_base_->self_index_;

	float u = data.b_0_, v = data.b_1_, w = data.b_2_;

	if(mesh_->is_smooth_ || mesh_->normals_exported_)
	{
		// assume the smoothed normals exist, if the mesh is smoothed; if they don't, fix this
		// assert(na > 0 && nb > 0 && nc > 0);

		Vec3 va = (na > 0) ? mesh_->getVertexNormal(na) : sp.ng_;
		Vec3 vb = (nb > 0) ? mesh_->getVertexNormal(nb) : sp.ng_;
		Vec3 vc = (nc > 0) ? mesh_->getVertexNormal(nc) : sp.ng_;

		sp.n_ = u * va + v * vb + w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = sp.ng_;

	if(mesh_->has_orco_)
	{
		// if the object is an instance, the vertex positions are the orcos
		Point3 const &p_0 = m_base_->mesh_->getVertex(pa + 1);
		Point3 const &p_1 = m_base_->mesh_->getVertex(pb + 1);
		Point3 const &p_2 = m_base_->mesh_->getVertex(pc + 1);

		sp.orco_p_ = u * p_0 + v * p_1 + w * p_2;

		sp.orco_ng_ = ((p_1 - p_0) ^ (p_2 - p_0)).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.has_orco_ = false;
		sp.orco_ng_ = sp.ng_;
	}

	Point3 const &p_0 = mesh_->getVertex(pa);
	Point3 const &p_1 = mesh_->getVertex(pb);
	Point3 const &p_2 = mesh_->getVertex(pc);

	if(mesh_->has_uv_)
	{
		size_t uvi = self_index * 3;
		const Uv &uv_1 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi]];
		const Uv &uv_2 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi + 1]];
		const Uv &uv_3 = mesh_->m_base_->uv_values_[mesh_->m_base_->uv_offsets_[uvi + 2]];

		//eh...u, v and w are actually the barycentric coords, not some UVs...quite annoying, i know...
		sp.u_ = u * uv_1.u_ + v * uv_2.u_ + w * uv_3.u_;
		sp.v_ = u * uv_1.v_ + v * uv_2.v_ + w * uv_3.v_;

		// calculate dPdU and dPdV
		float du_1 = uv_1.u_ - uv_3.u_;
		float du_2 = uv_2.u_ - uv_3.u_;
		float dv_1 = uv_1.v_ - uv_3.v_;
		float dv_2 = uv_2.v_ - uv_3.v_;
		float invdet, det = du_1 * dv_2 - dv_1 * du_2;

		if(std::fabs(det) > 1e-30f)
		{
			invdet = 1.f / det;
			Vec3 dp_1 = p_0 - p_2;
			Vec3 dp_2 = p_1 - p_2;
			sp.dp_du_ = (dv_2 * dp_1 - dv_1 * dp_2) * invdet;
			sp.dp_dv_ = (du_1 * dp_2 - du_2 * dp_1) * invdet;
		}
		else
		{
			Vec3 dp_1 = p_0 - p_2;
			Vec3 dp_2 = p_1 - p_2;
			createCs__((dp_2 ^ dp_1).normalize(), sp.dp_du_, sp.dp_dv_);
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => U = u, V = v; (arbitrary choice)
		Vec3 dp_1 = p_0 - p_2;
		Vec3 dp_2 = p_1 - p_2;
		sp.u_ = v + w;
		sp.v_ = w;
		sp.dp_du_ = dp_2 - dp_1;
		sp.dp_dv_ = -dp_2;
	}

	sp.dp_du_.normalize();
	sp.dp_dv_.normalize();

	sp.object_ = mesh_;
	sp.prim_num_ = self_index;
	sp.material_ = m_base_->material_;
	sp.p_ = hit;
	createCs__(sp.n_, sp.nu_, sp.nv_);
	Vec3 u_vec, v_vec;
	createCs__(sp.ng_, u_vec, v_vec);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = u_vec * sp.dp_du_;
	sp.ds_du_.y_ = v_vec * sp.dp_du_;
	sp.ds_du_.z_ = sp.ng_ * sp.dp_du_;
	sp.ds_dv_.x_ = u_vec * sp.dp_dv_;
	sp.ds_dv_.y_ = v_vec * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.ng_ * sp.dp_dv_;
	sp.ds_du_.normalize();
	sp.ds_dv_.normalize();
	sp.light_ = mesh_->m_base_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool TriangleInstance::clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		bool lower = axis & ~3;
		int axis_calc = axis & 3;
		double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		int res = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admit
		if(res > 1)
		{
			double t_points[3][3];

			Point3 const &a = mesh_->getVertex(m_base_->pa_);
			Point3 const &b = mesh_->getVertex(m_base_->pb_);
			Point3 const &c = mesh_->getVertex(m_base_->pc_);

			for(int i = 0; i < 3; ++i)
			{
				t_points[0][i] = a[i];
				t_points[1][i] = b[i];
				t_points[2][i] = c[i];
			}
			int res = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
			return (res == 0);
		}
		return (res == 0);
	}
	else // initial clip
	{
		double t_points[3][3];

		Point3 const &a = mesh_->getVertex(m_base_->pa_);
		Point3 const &b = mesh_->getVertex(m_base_->pb_);
		Point3 const &c = mesh_->getVertex(m_base_->pc_);

		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		int res = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (res == 0);
	}
	return true;
}

float TriangleInstance::surfaceArea() const
{
	Point3 const &a = mesh_->getVertex(m_base_->pa_);
	Point3 const &b = mesh_->getVertex(m_base_->pb_);
	Point3 const &c = mesh_->getVertex(m_base_->pc_);

	Vec3 edge_1, edge_2;
	edge_1 = b - a;
	edge_2 = c - a;
	return 0.5 * (edge_1 ^ edge_2).length();
}

void TriangleInstance::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	Point3 const &a = mesh_->getVertex(m_base_->pa_);
	Point3 const &b = mesh_->getVertex(m_base_->pb_);
	Point3 const &c = mesh_->getVertex(m_base_->pc_);

	float su_1 = fSqrt__(s_1);
	float u = 1.f - su_1;
	float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = getNormal();
}

//==========================================
// vTriangle_t methods, mosty c&p...
//==========================================

bool VTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	//Tomas Moller and Ben Trumbore ray intersection scheme
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	Vec3 edge_1, edge_2, tvec, pvec, qvec;
	float det, inv_det, u, v;
	edge_1 = b - a;
	edge_2 = c - a;
	pvec = ray.dir_ ^ edge_2;
	det = edge_1 * pvec;
	if(/*(det>-0.000001) && (det<0.000001)*/ det == 0.0) return false;
	inv_det = 1.0 / det;
	tvec = ray.from_ - a;
	u = (tvec * pvec) * inv_det;
	if(u < 0.0 || u > 1.0) return false;
	qvec = tvec ^ edge_1;
	v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.0) || ((u + v) > 1.0)) return false;
	*t = edge_2 * qvec * inv_det;
	data.b_1_ = u;
	data.b_2_ = v;
	return true;
}

Bound VTriangle::getBound() const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	Point3 l, h;
	l.x_ = Y_MIN_3(a.x_, b.x_, c.x_);
	l.y_ = Y_MIN_3(a.y_, b.y_, c.y_);
	l.z_ = Y_MIN_3(a.z_, b.z_, c.z_);
	h.x_ = Y_MAX_3(a.x_, b.x_, c.x_);
	h.y_ = Y_MAX_3(a.y_, b.y_, c.y_);
	h.z_ = Y_MAX_3(a.z_, b.z_, c.z_);
	return Bound(l, h);
}

void VTriangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	sp.ng_ = Vec3(normal_);
	// gives the index in triangle array, according to my latest informations
	// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
	int tri_index = this - &(mesh_->triangles_.front());
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	float u = data.b_0_, v = data.b_1_, w = data.b_2_;

	if(mesh_->is_smooth_)
	{
		Vec3 va(na_ > 0 ? mesh_->normals_[na_] : normal_), vb(nb_ > 0 ? mesh_->normals_[nb_] : normal_), vc(nc_ > 0 ? mesh_->normals_[nc_] : normal_);
		sp.n_ = u * va + v * vb + w * vc;
		sp.n_.normalize();
	}
	else sp.n_ = Vec3(normal_);

	if(mesh_->has_orco_)
	{
		sp.orco_p_ = u * mesh_->points_[pa_ + 1] + v * mesh_->points_[pb_ + 1] + w * mesh_->points_[pc_ + 1];
		sp.orco_ng_ = ((mesh_->points_[pb_ + 1] - mesh_->points_[pa_ + 1]) ^ (mesh_->points_[pc_ + 1] - mesh_->points_[pa_ + 1])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(mesh_->has_uv_)
	{
		auto uvi = mesh_->uv_offsets_.begin() + 3 * tri_index;
		int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		auto it = mesh_->uv_values_.begin();
		//eh...u, v and w are actually the barycentric coords, not some UVs...quite annoying, i know...
		sp.u_ = u * it[uvi_1].u_ + v * it[uvi_2].u_ + w * it[uvi_3].u_;
		sp.v_ = u * it[uvi_1].v_ + v * it[uvi_2].v_ + w * it[uvi_3].v_;

		// calculate dPdU and dPdV
		float du_1 = it[uvi_1].u_ - it[uvi_3].u_;
		float du_2 = it[uvi_2].u_ - it[uvi_3].u_;
		float dv_1 = it[uvi_1].v_ - it[uvi_3].v_;
		float dv_2 = it[uvi_2].v_ - it[uvi_3].v_;
		float invdet, det = du_1 * dv_2 - dv_1 * du_2;

		if(std::fabs(det) > 1e-30f)
		{
			invdet = 1.f / det;
			Vec3 dp_1 = mesh_->points_[pa_] - mesh_->points_[pc_];
			Vec3 dp_2 = mesh_->points_[pb_] - mesh_->points_[pc_];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			sp.dp_du_ = Vec3(0.f);
			sp.dp_dv_ = Vec3(0.f);
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => U = u, V = v; (arbitrary choice)
		sp.u_ = u;
		sp.v_ = v;
		sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
		sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
	}

	sp.prim_num_ = tri_index;
	sp.material_ = material_;
	sp.p_ = hit;
	createCs__(sp.n_, sp.nu_, sp.nv_);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y_ = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z_ = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x_ = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y_ = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.n_ * sp.dp_dv_;
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}

bool VTriangle::intersectsBound(ExBound &eb) const
{
	double t_points[3][3];
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	for(int j = 0; j < 3; ++j)
	{
		t_points[0][j] = a[j];
		t_points[1][j] = b[j];
		t_points[2][j] = c[j];
	}
	// triBoxOverlap() is in src/yafraycore/tribox3_d.cc!
	return triBoxOverlap__(eb.center_, eb.half_size_, (double **) t_points);
}

bool VTriangle::clipToBound(double bound[2][3], int axis, Bound &clipped, void *d_old, void *d_new) const
{
	if(axis >= 0) // re-clip
	{
		bool lower = axis & ~3;
		int axis_calc = axis & 3;
		double split = lower ? bound[0][axis_calc] : bound[1][axis_calc];
		int res = triPlaneClip__(split, axis_calc, lower, clipped, d_old, d_new);
		// if an error occured due to precision limits...ugly solution i admitt
		if(res > 1) goto WHOOPS;
		return (res == 0) ? true : false;
	}
	else // initial clip
	{
		//		std::cout << "+";
WHOOPS:
		//		std::cout << "!";
		double t_points[3][3];
		const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
		for(int i = 0; i < 3; ++i)
		{
			t_points[0][i] = a[i];
			t_points[1][i] = b[i];
			t_points[2][i] = c[i];
		}
		int res = triBoxClip__(bound[0], bound[1], t_points, clipped, d_new);
		return (res == 0) ? true : false;
	}
	return true;
}

float VTriangle::surfaceArea() const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	Vec3 edge_1, edge_2;
	edge_1 = b - a;
	edge_2 = c - a;
	return 0.5 * (edge_1 ^ edge_2).length();
}

void VTriangle::sample(float s_1, float s_2, Point3 &p, Vec3 &n) const
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	float su_1 = fSqrt__(s_1);
	float u = 1.f - su_1;
	float v = s_2 * su_1;
	p = u * a + v * b + (1.f - u - v) * c;
	n = Vec3(normal_);
}

void VTriangle::recNormal()
{
	const Point3 &a = mesh_->points_[pa_], &b = mesh_->points_[pb_], &c = mesh_->points_[pc_];
	normal_ = ((b - a) ^ (c - a)).normalize();
}

//==========================================
// bsTriangle_t methods
//==========================================

bool BsTriangle::intersect(const Ray &ray, float *t, IntersectData &data) const
{
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
	float tc = 1.f - ray.time_;
	float b_1 = tc * tc, b_2 = 2.f * ray.time_ * tc, b_3 = ray.time_ * ray.time_;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];

	Vec3 edge_1, edge_2, tvec, pvec, qvec;
	float det, inv_det, u, v;
	edge_1 = b - a;
	edge_2 = c - a;
	pvec = ray.dir_ ^ edge_2;
	det = edge_1 * pvec;
	if(/*(det>-0.000001) && (det<0.000001)*/ det == 0.0) return false;
	inv_det = 1.0 / det;
	tvec = ray.from_ - a;
	u = (tvec * pvec) * inv_det;
	if(u < 0.0 || u > 1.0) return false;
	qvec = tvec ^ edge_1;
	v = (ray.dir_ * qvec) * inv_det;
	if((v < 0.0) || ((u + v) > 1.0)) return false;
	*t = edge_2 * qvec * inv_det;

	data.b_1_ = u;
	data.b_1_ = v;
	data.t_ = ray.time_;
	return true;
}

Bound BsTriangle::getBound() const
{
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
	Point3 amin, bmin, cmin, amax, bmax, cmax;
	amin.x_ = Y_MIN_3(an[0].x_, an[1].x_, an[2].x_);
	amin.y_ = Y_MIN_3(an[0].y_, an[1].y_, an[2].y_);
	amin.z_ = Y_MIN_3(an[0].z_, an[1].z_, an[2].z_);
	bmin.x_ = Y_MIN_3(bn[0].x_, bn[1].x_, bn[2].x_);
	bmin.y_ = Y_MIN_3(bn[0].y_, bn[1].y_, bn[2].y_);
	bmin.z_ = Y_MIN_3(bn[0].z_, bn[1].z_, bn[2].z_);
	cmin.x_ = Y_MIN_3(cn[0].x_, cn[1].x_, cn[2].x_);
	cmin.y_ = Y_MIN_3(cn[0].y_, cn[1].y_, cn[2].y_);
	cmin.z_ = Y_MIN_3(cn[0].z_, cn[1].z_, cn[2].z_);
	amax.x_ = Y_MAX_3(an[0].x_, an[1].x_, an[2].x_);
	amax.y_ = Y_MAX_3(an[0].y_, an[1].y_, an[2].y_);
	amax.z_ = Y_MAX_3(an[0].z_, an[1].z_, an[2].z_);
	bmax.x_ = Y_MAX_3(bn[0].x_, bn[1].x_, bn[2].x_);
	bmax.y_ = Y_MAX_3(bn[0].y_, bn[1].y_, bn[2].y_);
	bmax.z_ = Y_MAX_3(bn[0].z_, bn[1].z_, bn[2].z_);
	cmax.x_ = Y_MAX_3(cn[0].x_, cn[1].x_, cn[2].x_);
	cmax.y_ = Y_MAX_3(cn[0].y_, cn[1].y_, cn[2].y_);
	cmax.z_ = Y_MAX_3(cn[0].z_, cn[1].z_, cn[2].z_);
	Point3 l, h;
	l.x_ = Y_MIN_3(amin.x_, bmin.x_, cmin.x_);
	l.y_ = Y_MIN_3(amin.y_, bmin.y_, cmin.y_);
	l.z_ = Y_MIN_3(amin.z_, bmin.z_, cmin.z_);
	h.x_ = Y_MAX_3(amax.x_, bmax.x_, cmax.x_);
	h.y_ = Y_MAX_3(amax.y_, bmax.y_, cmax.y_);
	h.z_ = Y_MAX_3(amax.z_, bmax.z_, cmax.z_);
	return Bound(l, h);
}

void BsTriangle::getSurface(SurfacePoint &sp, const Point3 &hit, IntersectData &data) const
{
	// recalculating the points is not really the nicest solution...
	const Point3 *an = &mesh_->points_[pa_], *bn = &mesh_->points_[pb_], *cn = &mesh_->points_[pc_];
	float time = data.t_;
	float tc = 1.f - time;
	float b_1 = tc * tc, b_2 = 2.f * time * tc, b_3 = time * time;
	const Point3 a = b_1 * an[0] + b_2 * an[1] + b_3 * an[2];
	const Point3 b = b_1 * bn[0] + b_2 * bn[1] + b_3 * bn[2];
	const Point3 c = b_1 * cn[0] + b_2 * cn[1] + b_3 * cn[2];

	sp.ng_ = ((b - a) ^ (c - a)).normalize();
	// the "u" and "v" in triangle intersection code are actually "v" and "w" when u=>p1, v=>p2, w=>p3
	float u = data.b_0_, v = data.b_1_, w = data.b_2_;

	//todo: calculate smoothed normal...
	/* if(mesh->is_smooth || mesh->normals_exported)
	{
		vector3d_t va(na>0? mesh->normals[na] : normal), vb(nb>0? mesh->normals[nb] : normal), vc(nc>0? mesh->normals[nc] : normal);
		sp.N = u*va + v*vb + w*vc;
		sp.N.normalize();
	}
	else  */sp.n_ = sp.ng_;

	if(mesh_->has_orco_)
	{
		sp.orco_p_ = u * mesh_->points_[pa_ + 1] + v * mesh_->points_[pb_ + 1] + w * mesh_->points_[pc_ + 1];
		sp.orco_ng_ = ((mesh_->points_[pb_ + 1] - mesh_->points_[pa_ + 1]) ^ (mesh_->points_[pc_ + 1] - mesh_->points_[pa_ + 1])).normalize();
		sp.has_orco_ = true;
	}
	else
	{
		sp.orco_p_ = hit;
		sp.orco_ng_ = sp.ng_;
		sp.has_orco_ = false;
	}
	if(mesh_->has_uv_)
	{
		//		static bool test=true;

		// gives the index in triangle array, according to my latest informations
		// it _should be_ safe to rely on array-like contiguous memory in std::vector<>!
		unsigned int tri_index = this - &(mesh_->s_triangles_.front());
		auto uvi = mesh_->uv_offsets_.begin() + 3 * tri_index;
		int uvi_1 = *uvi, uvi_2 = *(uvi + 1), uvi_3 = *(uvi + 2);
		auto it = mesh_->uv_values_.begin();
		//eh...u, v and w are actually the barycentric coords, not some UVs...quite annoying, i know...
		sp.u_ = u * it[uvi_1].u_ + v * it[uvi_2].u_ + w * it[uvi_3].u_;
		sp.v_ = u * it[uvi_1].v_ + v * it[uvi_2].v_ + w * it[uvi_3].v_;

		// calculate dPdU and dPdV
		float du_1 = it[uvi_1].u_ - it[uvi_3].u_;
		float du_2 = it[uvi_2].u_ - it[uvi_3].u_;
		float dv_1 = it[uvi_1].v_ - it[uvi_3].v_;
		float dv_2 = it[uvi_2].v_ - it[uvi_3].v_;
		float invdet, det = du_1 * dv_2 - dv_1 * du_2;

		if(std::fabs(det) > 1e-30f)
		{
			invdet = 1.f / det;
			Vec3 dp_1 = mesh_->points_[pa_] - mesh_->points_[pc_];
			Vec3 dp_2 = mesh_->points_[pb_] - mesh_->points_[pc_];
			sp.dp_du_ = (dv_2 * invdet) * dp_1 - (dv_1 * invdet) * dp_2;
			sp.dp_dv_ = (du_1 * invdet) * dp_2 - (du_2 * invdet) * dp_1;
		}
		else
		{
			sp.dp_du_ = Vec3(0.f);
			sp.dp_dv_ = Vec3(0.f);
		}
	}
	else
	{
		// implicit mapping, p0 = 0/0, p1 = 1/0, p2 = 0/1 => U = u, V = v; (arbitrary choice)
		sp.u_ = u;
		sp.v_ = v;
		sp.dp_du_ = mesh_->points_[pb_] - mesh_->points_[pa_];
		sp.dp_dv_ = mesh_->points_[pc_] - mesh_->points_[pa_];
	}
	sp.material_ = material_;
	sp.p_ = hit;
	createCs__(sp.n_, sp.nu_, sp.nv_);
	// transform dPdU and dPdV in shading space
	sp.ds_du_.x_ = sp.nu_ * sp.dp_du_;
	sp.ds_du_.y_ = sp.nv_ * sp.dp_du_;
	sp.ds_du_.z_ = sp.n_ * sp.dp_du_;
	sp.ds_dv_.x_ = sp.nu_ * sp.dp_dv_;
	sp.ds_dv_.y_ = sp.nv_ * sp.dp_dv_;
	sp.ds_dv_.z_ = sp.n_ * sp.dp_dv_;
	sp.light_ = mesh_->light_;
	sp.has_uv_ = mesh_->has_uv_;
}


END_YAFRAY
