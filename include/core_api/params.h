#ifndef Y_PARAMS_H
#define Y_PARAMS_H

#include "color.h"
#include "vector3d.h"
#include "matrix4.h"

#include <map>
#include <string>

__BEGIN_YAFRAY


#define TYPE_INT    1
#define TYPE_BOOL   2
#define TYPE_FLOAT  3
#define TYPE_STRING 4
#define TYPE_POINT  5
#define TYPE_COLOR  6
#define TYPE_NONE   -1

/*! a class that can hold exactly one value of a range types.
*/
class YAFRAYCORE_EXPORT parameter_t
{
	public:
		parameter_t(const std::string &s): used(false), vtype(TYPE_STRING) { str=s; }
		parameter_t(int i): used(false), vtype(TYPE_INT) { ival=i; }
		parameter_t(bool b): used(false), vtype(TYPE_BOOL) { bval=b; }
		parameter_t(float f): used(false), vtype(TYPE_FLOAT) { fval=f; }
		parameter_t(double f): used(false), vtype(TYPE_FLOAT) { fval=f; }
		parameter_t(const colorA_t &c): used(false), vtype(TYPE_COLOR) { C[0]=c.R, C[1]=c.G, C[2]=c.B, C[3]=c.A; }
		parameter_t(const point3d_t &p): used(false), vtype(TYPE_POINT){ P[0]=p.x, P[1]=p.y, P[2]=p.z; }
//		parameter_t(const parameter_t &p);
		parameter_t(): used(false), vtype(TYPE_NONE) {};
		~parameter_t(){};
		
		// return parameter value in reference parameter, return true if
		// the parameter type matches, false otherwise.
		bool getVal(std::string &s) const {used=true; if(vtype==TYPE_STRING){s=str; return true;}return false;};
		bool getVal(const std::string *&s) const {used=true; if(vtype==TYPE_STRING){s=&str; return true;}return false;};
		bool getVal(int &i)			const {used=true; if(vtype==TYPE_INT)  {i=ival; return true;} return false;};
		bool getVal(bool &b)		const {used=true; if(vtype==TYPE_BOOL)  {b=bval; return true;} return false;};
		bool getVal(float &f)		const {used=true; if(vtype==TYPE_FLOAT){f=(float)fval; return true;} return false;};
		bool getVal(double &f)		const {used=true; if(vtype==TYPE_FLOAT){f=fval; return true;} return false;};
		bool getVal(point3d_t &p)	const {used=true; if(vtype==TYPE_POINT){p.x=P[0], p.y=P[1], p.z=P[2]; return true;} return false;};
		bool getVal(color_t &c)		const {used=true; if(vtype==TYPE_COLOR){c.R=C[0], c.G=C[1], c.B=C[2]; return true;} return false;};
		bool getVal(colorA_t &c)	const {used=true; if(vtype==TYPE_COLOR){c.R=C[0], c.G=C[1], c.B=C[2], c.A=C[3]; return true;} return false;};
		//! return the type of the parameter_t
		int type()const { return vtype; }
		// operator= assigns new value, be aware that this may change the parameter type!
		parameter_t &operator = (const std::string &s) { vtype=TYPE_STRING; str=s; return *this; }
		parameter_t &operator = (int i) { vtype=TYPE_INT; ival=i; return *this; }
		parameter_t &operator = (float f) { vtype=TYPE_FLOAT; fval=f; return *this; }
		parameter_t &operator = (const point3d_t &p) { vtype=TYPE_POINT; P[0]=p.x, P[1]=p.y, P[2]=p.z; return *this; }
		parameter_t &operator = (const colorA_t &c) { vtype=TYPE_COLOR; C[0]=c.R, C[1]=c.G, C[2]=c.B, C[3]=c.A; return *this; }
		
		mutable bool used; //! indicate whether the parameter was read (sucessfully) before
	protected:
		std::string str;
		//point3d_t P;
		//colorA_t C;
		union
		{
			int ival;
			double fval;
			bool bval;
			CFLOAT C[4];
			PFLOAT P[4];
			//PFLOAT matrix[4][4];
		};
		int vtype; //!< type of the stored value
};

class YAFRAYCORE_EXPORT paraMap_t
{
	public:
		paraMap_t(){}
		//! template function to get a value, available types are those of parameter_t::getVal()
		template <class T>
		bool getParam(const std::string &name, T &val) const
		{
			std::map<std::string,parameter_t>::const_iterator i=dicc.find(name);
			if(i != dicc.end() ) return i->second.getVal(val);
			return false;
		}
		
		bool getMatrix(const std::string &name, matrix4x4_t &m) const
		{
			std::map<std::string,matrix4x4_t>::const_iterator i=mdicc.find(name);
			if(i != mdicc.end() ){ m = i->second; return true; }
			return false;
		}
//		virtual bool includes(const std::string &label,int type)const;
//		virtual void checkUnused(const std::string &env)const;
		parameter_t & operator [] (const std::string &key){ return dicc[key]; };
		void setMatrix(const std::string &key, const matrix4x4_t &m){ mdicc[key] = m; }
		
		void clear() { dicc.clear(); mdicc.clear(); }
		//! get the actualy parameter dictionary;
		/*	Usefull e.g. to dump it to file, since we don't provide iterators etc (yet)... */
		const std::map<std::string,parameter_t>* getDict() const{ return &dicc; }
		const std::map<std::string,matrix4x4_t>* getMDict() const{ return &mdicc; }
		virtual ~paraMap_t(){};
	protected:
		std::map<std::string,parameter_t> dicc;
		std::map<std::string,matrix4x4_t> mdicc;
};


__END_YAFRAY
#endif // Y_PARAMS_H
