/******************************************************************************
 * Top contributors (to current version):
 *   Mudathir Mohamed
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2021 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * The cvc5 Java API.
 */

#include <jni.h>

#ifndef CVC5__JAVA_API_H
#define CVC5__JAVA_API_H

#define CVC5_JAVA_API_TRY_CATCH_BEGIN \
  try                                 \
  {
#define CVC5_JAVA_API_TRY_CATCH_END(env)                             \
  }                                                                  \
  catch (const CVC5ApiRecoverableException& e)                       \
  {                                                                  \
    jclass exceptionClass =                                          \
        env->FindClass("cvc5/CVC5ApiRecoverableException");          \
    env->ThrowNew(exceptionClass, e.what());                         \
  }                                                                  \
  catch (const CVC5ApiException& e)                                  \
  {                                                                  \
    jclass exceptionClass = env->FindClass("cvc5/CVC5ApiException"); \
    env->ThrowNew(exceptionClass, e.what());                         \
  }
#define CVC5_JAVA_API_TRY_CATCH_END_RETURN(env, returnValue) \
  CVC5_JAVA_API_TRY_CATCH_END(env)                           \
  return returnValue;
#endif  // CVC5__JAVA_API_H

/**
 * Convert pointers coming from Java to cvc5 objects
 * @tparam T cvc5 class (Term, Sort, Grammar, etc)
 * @param env jni environment
 * @param jPointers pointers coming from java
 * @return a vector of cvc5 objects
 */
template <class T>
std::vector<T> getObjectsFromPointers(JNIEnv* env, jlongArray jPointers)
{
  // get the size of pointers
  jsize size = env->GetArrayLength(jPointers);
  // allocate a buffer for c pointers
  std::vector<jlong> cPointers(size);
  // copy java array to the buffer
  env->GetLongArrayRegion(jPointers, 0, size, cPointers.data());
  // copy the terms into a vector
  std::vector<T> objects;
  for (jlong pointer : cPointers)
  {
    T* term = reinterpret_cast<T*>(pointer);
    objects.push_back(*term);
  }
  return objects;
}

/**
 * Convert cvc5 objects into pointers
 * @tparam T cvc5 class (Term, Sort, Grammar, etc)
 * @param env jni environment
 * @param objects cvc5 objects
 * @return jni array of pointers
 */
template <class T>
jlongArray getPointersFromObjects(JNIEnv* env, const std::vector<T>& objects)
{
  std::vector<jlong> pointers(objects.size());
  for (size_t i = 0; i < objects.size(); i++)
  {
    pointers[i] = reinterpret_cast<jlong>(new T(objects[i]));
  }
  jlongArray ret = env->NewLongArray(objects.size());
  env->SetLongArrayRegion(ret, 0, objects.size(), pointers.data());
  return ret;
}