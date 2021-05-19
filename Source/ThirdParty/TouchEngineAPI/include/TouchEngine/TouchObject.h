/* Shared Use License: This file is owned by Derivative Inc. (Derivative)
* and can only be used, and/or modified for use, in conjunction with
* Derivative's TouchDesigner software, and only if you are a licensee who has
* accepted Derivative's TouchDesigner license or assignment agreement
* (which also govern the use of this file). You may share or redistribute
* a modified version of this file provided the following conditions are met:
*
* 1. The shared file or redistribution must retain the information set out
* above and this list of conditions.
* 2. Derivative's name (Derivative Inc.) or its trademarks may not be used
* to endorse or promote products derived from this file without specific
* prior written permission from Derivative.
*/


#ifndef TouchObject_h
#define TouchObject_h

#ifdef __cplusplus

#include <TouchEngine/TouchEngine.h>
#include <type_traits>

/*
* TouchObject wraps TEObject types, managing reference counting through TERetain and TERelease
* 
* It has two methods for transferring an existing TEObject in:
* "take" - takes over ownership of a TEObject (no call to TERetain on take, TERelease when the TouchObject is destroyed)
* "set" - adds a reference, becoming an additional owner of the TEObject (calls TERetain on set, TERelease when the TouchObject is destroyed)
* 
* Construct a TouchObject from a TouchEngine function returning a TEObject, taking ownership of the object:
* 
*	TouchObject<TETable> table = TouchObject::make_take(TETableCreate());
* 
* Construct a TouchObject from a TouchEngine function returning a value through a parameter, taking ownership of the object:
*
*	TouchObject<TEDXGITexture> texture; // empty
*	TEResult result = TEInstanceLinkGetTextureValue(myInstance, identifier.c_str(), TELinkValueCurrent, texture.take());
*	if (result == TEResultSuccess)
*	{
*		// texture now contains a TETexture
*	}
*	// the TouchObject will release the TETexture when it goes out of scope
* 
* You can pass a TouchObject directly to TouchEngine functions:
* 
*	TouchObject<TETable> table = TouchObject::make_take(TETableCreate());
*	TETableResize(table, 3, 2);
* 
* TouchObjects are copyable and assignable and you can use them in other objects, etc:
* 
*	std::vector<TouchObject<TETexture>> textures;
* 
*/

/*
* Utility for TouchObject
*/
template <typename T, typename U, typename = void>
struct TouchIsMemberOf : std::false_type
{};

template <typename T, typename U>
struct TouchIsMemberOf<T, U, typename std::enable_if_t<
	std::is_same<T, TEObject>::value ||
	(std::is_same<T, TETexture>::value && (
		std::is_same<U, TEOpenGLTexture>::value ||
		std::is_same<U, TEDXGITexture>::value ||
		std::is_same<U, TED3D11Texture>::value)) ||
	(std::is_same<T, TEGraphicsContext>::value && (
		std::is_same<U, TEOpenGLContext>::value ||
		std::is_same<U, TED3D11Context>::value))>> : std::true_type
{};

/*
* Wrapper for TEObjects
*/
template <typename T>
class TouchObject
{
public:
	/*
	* Empty TouchObject
	*/
	TouchObject()
	{	}
	/*
	* Empty TouchObject
	*/
	TouchObject(nullptr_t)
	{	}

	~TouchObject()
	{
		TERelease(&myObject);
	}
	
	TouchObject(const TouchObject<T>& o)
		: myObject(static_cast<T *>(TERetain(o.myObject)))
	{	}
	
	TouchObject(TouchObject<T>&& o)
		: myObject(o.myObject)
	{
		o.myObject = nullptr;
	}
	
	template <typename O, std::enable_if_t<TouchIsMemberOf<T, O>::value, int > = 0 >
	TouchObject(const TouchObject<O>& o)
		: myObject(static_cast<T *>(TERetain(o.get())))
	{	};
	
	TouchObject& operator=(const TouchObject<T>& o)
	{
		if (&o != this)
		{
			TERetain(o.myObject);
			TERelease(&myObject);
			myObject = o.myObject;
		}
		return *this;
	}
	
	TouchObject<T>& operator=(TouchObject<T>&& o)
	{
		TERelease(&myObject);
		myObject = o.myObject;
		o.myObject = nullptr;
		return *this;
	}
	
	template <typename O, std::enable_if_t<TouchIsMemberOf<T, O>::value, int > = 0 >
	TouchObject& operator=(const TouchObject<O>& o)
	{
		set(o.get());
		return *this;
	}
	
	void reset()
	{
		TERelease(&myObject);
	}
	
	operator T*() const
	{
		return myObject;
	}
	
	T* get() const
	{
		return myObject;
	}
	
	/*
	* Use set(T*) to become an additional owner of a TEObject.
	*/
	void set(T* o)
	{
		TERetain(o);
		TERelease(&myObject);
		myObject = o;
	}
	
	/*
	* Use take() to take ownership from functions which return a TEObject through a parameter.
	* This TouchObject takes ownership of the TEObject when it is returned.
	*/
	T** take()
	{
		TERelease(&myObject);
		return &myObject;
	}
	/*
	* Use take(T*) to transfer ownership of a TEObject to this TouchObject.
	*/
	void take(T* o)
	{
		TERelease(&myObject);
		myObject = o;
	}
	static TouchObject<T> make_take(T* o)
	{
		TouchObject<T> obj;
		obj.take(o);
		return obj;
	}
	static TouchObject<T> make_set(T* o)
	{
		TouchObject<T> obj;
		obj.set(o);
		return obj;
	}
private:
	T* myObject{ nullptr };

};

#endif

#endif
