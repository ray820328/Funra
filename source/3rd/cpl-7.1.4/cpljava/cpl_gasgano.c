/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include <jni.h>

#include "ltdl.h"

#include "cxmessages.h"

#include "cpl_init.h"
#include "cpl_frame.h"
#include "cpl_frameset.h"
#include "cpl_msg.h"
#include "cpl_parameter.h"
#include "cpl_parameterlist.h"
#include "cpl_plugin.h"
#include "cpl_pluginlist.h"
#include "cpl_recipe.h"
#include "cpl_dfs.h"
#include "cpl_errorstate.h"
#include "cpl_version.h"
#include "cpl_memory.h"

#include "org_eso_cpl_jni_CPLControl.h"
#include "org_eso_cpl_jni_JNIParameterImp.h"
#include "org_eso_cpl_jni_JNIRecipe.h"
#include "org_eso_cpl_jni_LibraryLoader.h"
#include "org_eso_cpl_jni_PluginLibrary.h"

#if defined (HAVE_ORG_ESO_CPL_TEST_NATIVE_TEST_H) && \
    defined (ENABLE_NATIVE_TESTS)
#  include "org_eso_cpl_test_NativeTest.h"
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

/*
 *Library version.
 */

#define JAVACPL_VERSION "1.0"


/*
 * Macro for executing one of the named cpl_plugin_func functions defined
 * in the cpl_plugin structure.  Xname gives the name of the member of
 * the cpl_plugin structure to execute.
 */

#define EXECUTE_CPL_PLUGIN_FUNC(env, this, Xname) \
{ \
    jbyteArray jState = NULL; \
    cpl_plugin *plugin = NULL; \
    int ok = 1; \
    int status; \
    \
    /* Get the pointer to the function to execute. */ \
    ok = ok && \
        (jState = (*env)->GetObjectField(env, this, JNIRecipeStateField)) && \
        (plugin = (cpl_plugin *) \
                    (*env)->GetByteArrayElements(env, jState, NULL)); \
    \
    /* Execute the function if it is non-NULL. */ \
    if (ok) { \
        const cpl_plugin_func cplfunc = plugin->Xname; \
        if (cplfunc) { \
            currentEnv = env; \
            status = (*cplfunc)(plugin); \
            currentEnv = NULL; \
} \
        else { \
            status = 0; \
} \
} \
    else { \
        status = -1; \
} \
    \
    /* Release array elements. */ \
    if (plugin) { \
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) plugin, 0); \
} \
    \
    /* Return the execution status. */ \
    return status; \
}


/*
 * Static variables.
 */

static int isConstantsSetup = 0;
static int isHandlersSetup = 0;

static cpl_errorstate inistate;

static JNIEnv *currentEnv;

static jclass AssertionErrorClass;
static jclass BooleanClass;
static jclass DoubleClass;
static jclass FileClass;
static jclass IllegalArgumentExceptionClass;
static jclass IllegalStateExceptionClass;
static jclass IntegerClass;
static jclass LinkageErrorClass;
static jclass NullPointerExceptionClass;
static jclass ObjectClass;
static jclass PrintStreamClass;
static jclass RuntimeExceptionClass;
static jclass StringClass;
static jclass BadAPIPluginClass;
static jclass CPLControlClass;
static jclass CPLExceptionClass;
static jclass EnumConstraintClass;
static jclass FrameClass;
static jclass FrameGroupClass;
static jclass FrameLevelClass;
static jclass FrameTypeClass;
static jclass JNIParameterImpClass;
static jclass JNIRecipeClass;
static jclass LTDLExceptionClass;
static jclass NonRecipePluginClass;
static jclass OtherPluginClass;
static jclass ParameterClass;
static jclass ParameterTypeClass;
static jclass ParameterValueExceptionClass;
static jclass PluginLibraryClass;
static jclass RangeConstraintClass;

static jmethodID BadAPIPluginConstructor;
static jmethodID DoubleConstructor;
static jmethodID EnumConstraintConstructor;
static jmethodID FileConstructor;
static jmethodID FrameConstructor;
static jmethodID IntegerConstructor;
static jmethodID JNIParameterImpConstructor;
static jmethodID JNIRecipeConstructor;
static jmethodID LTDLExceptionConstructor;
static jmethodID NonRecipePluginConstructor;
static jmethodID ParameterConstructor;
static jmethodID PluginLibraryConstructor;
static jmethodID RangeConstraintConstructor;
static jmethodID BooleanBooleanValueMethod;
static jmethodID CPLControlErrMessageMethod;
static jmethodID CPLControlLogMessageMethod;
static jmethodID CPLControlOutMessageMethod;
static jmethodID DoubleDoubleValueMethod;
static jmethodID IntegerIntValueMethod;
static jmethodID PrintStreamPrintlnMethod;

static jfieldID FrameCanonicalPathField;
static jfieldID FrameGroupField;
static jfieldID FrameLevelField;
static jfieldID FrameTagField;
static jfieldID FrameTypeField;
static jfieldID FrameGroupIDNameField;
static jfieldID FrameLevelIDNameField;
static jfieldID FrameTypeIDNameField;
static jfieldID JNIParameterImpContextField;
static jfieldID JNIParameterImpHelpField;
static jfieldID JNIParameterImpNameField;
static jfieldID JNIParameterImpStateField;
static jfieldID JNIParameterImpTagField;
static jfieldID JNIRecipeAuthorField;
static jfieldID JNIRecipeCopyrightField;
static jfieldID JNIRecipeDescriptionField;
static jfieldID JNIRecipeEmailField;
static jfieldID JNIRecipeNameField;
static jfieldID JNIRecipeStateField;
static jfieldID JNIRecipeSynopsisField;
static jfieldID JNIRecipeVersionField;
static jfieldID PluginLibraryStateField;

static jobject BooleanFALSEConstant;
static jobject BooleanTRUEConstant;
static jobject FrameGroupRAWConstant;
static jobject FrameGroupCALIBConstant;
static jobject FrameGroupPRODUCTConstant;
static jobject FrameLevelTEMPORARYConstant;
static jobject FrameLevelINTERMEDIATEConstant;
static jobject FrameLevelFINALConstant;
static jobject FrameTypeIMAGEConstant;
static jobject FrameTypeMATRIXConstant;
static jobject FrameTypeTABLEConstant;
static jobject ParameterTypeBOOLConstant;
static jobject ParameterTypeDOUBLEConstant;
static jobject ParameterTypeINTConstant;
static jobject ParameterTypeSTRINGConstant;

static jstring VersionString;


/*
 * Static function prototypes.
 */

static int doExec(JNIEnv *env, jobject recipe);
static int ensureConstantsSetup(JNIEnv *env);
static int ensureHandlersSetup(JNIEnv *env);

static void out_message_handler(const cxchar *msg);
static void err_message_handler(const cxchar *msg);
static void default_log_message_handler(const cxchar *msg,
                                        cx_log_level_flags flags,
                                        const cxchar *, cxptr);

static cpl_error_code setCplMessaging(cpl_boolean);

static cpl_frame_group getFrameGroupID(JNIEnv *env, jobject jFgroup);
static cpl_frame_level getFrameLevelID(JNIEnv *env, jobject jFlevel);
static cpl_frame_type getFrameTypeID(JNIEnv *env, jobject jFtype);
static cpl_frame *makeCplFrame(JNIEnv *env, jobject jFrame);
static cpl_frameset *makeFrameSet(JNIEnv *env, jobjectArray jFrames);

static jobject makeBoolean(int val);
static jobject makeDouble(JNIEnv *env, double val);
static jobject makeInteger(JNIEnv *env, int val);
static jobject makeJavaFrame(JNIEnv *env, cpl_frame *frame);
static jobject makeJNIRecipe(JNIEnv *env, const cpl_recipe *recipe,
                             jobject jPluglib);
static jobject makeParameter(JNIEnv *env, cpl_parameter *param,
                             jobject jRecipe);
static jobject makePluginLibrary(JNIEnv *env, jstring jLocation,
                                 lt_dlhandle dlhandle);
static jobject makeString(JNIEnv *env, const char *val);

static jobjectArray makeFrameArray(JNIEnv *env, cpl_frameset *fset);
static jobjectArray makeParameterArray(JNIEnv *env,
                                       cpl_parameterlist *parlist,
                                       jobject jRecipe);

static jthrowable makeLTDLException(JNIEnv *env);



/*
 * CPLControl native methods.
 */

/*
 * Ensures that all the static initialisation required for this library
 * has been done.
 */

JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_CPLControl_nativeEnsureSetup(JNIEnv *env, jclass class)
{
    if (class) {
        ensureConstantsSetup(env);
        ensureHandlersSetup(env);
    }
}


JNIEXPORT jstring JNICALL
Java_org_eso_cpl_jni_CPLControl_nativeGetVersion(JNIEnv *env, jclass class)
{
    return env && class ? VersionString : NULL;
}



/*
 * LibraryLoader native methods.
 */

/*
 * Calls lt_dlinit.
 */

JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_LibraryLoader_nativeLTDLInit(JNIEnv *env, jclass class)
{
     if (class == NULL || lt_dlinit()) {
         (*env)->Throw(env, makeLTDLException(env));
     }
}


/*
 * Calls lt_dlexit.
 */

JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_LibraryLoader_nativeLTDLExit(JNIEnv *env, jclass class)
{
    if (lt_dlexit() || class == NULL) {
        (*env)->Throw(env, makeLTDLException(env));
    }
}


/*
 * Attempts to open a library using LTDL with a given user search path,
 */

JNIEXPORT jobject JNICALL
Java_org_eso_cpl_jni_LibraryLoader_nativeMakeLibrary(JNIEnv *env,
                                                     jclass class,
                                                     jstring jFilename,
                                                     jstring jPath)
{
    const char *filename = NULL;
    const char *path = NULL;
    lt_dlhandle dlhandle = NULL;
    jthrowable error = NULL;


    if (class == NULL) {
        error = makeLTDLException(env);
    }

    /*
     * Get C strings from the java ones.
     */

    if (jFilename) {
        filename = (*env)->GetStringUTFChars(env, jFilename, NULL);
    }
    if (jPath) {
        path = (*env)->GetStringUTFChars(env, jPath, NULL);
    }


    /*
     * Attempt the open.
     */

    if (path) {
        if (lt_dlsetsearchpath(path)) {
            error = makeLTDLException(env);
        }
    }
    if (!error) {
        dlhandle = lt_dlopen(filename);
        if (!dlhandle) {
            error = makeLTDLException(env);
        }
    }


    /*
     * Release the strings.
     */

    if (filename) {
        (*env)->ReleaseStringUTFChars(env, jFilename, filename);
    }
    if (path) {
        (*env)->ReleaseStringUTFChars(env, jPath, path);
    }


    /*
     * Throw an exception if we picked one up.
     */

    if (error) {
        (*env)->Throw(env, error);
        return NULL;
    }


    /*
     * Otherwise return a PluginLibrary object based on the returned handle.
     */

    return dlhandle ? makePluginLibrary(env, jFilename, dlhandle) : NULL;
}



/*
 * JNIRecipe native methods.
 */

JNIEXPORT jint JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeInitialize(JNIEnv *env, jobject this)
{
   EXECUTE_CPL_PLUGIN_FUNC(env, this, initialize)
}


JNIEXPORT jint JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeDeinitialize(JNIEnv *env, jobject this)
{
   EXECUTE_CPL_PLUGIN_FUNC(env, this, deinitialize)
}


int doExec(JNIEnv *env, jobject recipe) {

    jbyteArray jState = NULL;
    cpl_plugin *plugin = NULL;
    int ok = 1;
    int status;

    /* Get the pointer to the function to execute. */
    ok = ok &&
        (jState = (*env)->GetObjectField(env, recipe, JNIRecipeStateField)) &&
        (plugin = (cpl_plugin *)
         (*env)->GetByteArrayElements(env, jState, NULL));

    /* Execute the function if it is non-NULL. */
    if (ok) {

        const cpl_plugin_func cplfunc = cpl_plugin_get_exec(plugin);

        if (cplfunc) {
            currentEnv = env;
            status = (*cplfunc)(plugin);
            currentEnv = NULL;

            if (cpl_plugin_get_type(plugin) & CPL_PLUGIN_TYPE_RECIPE) {

                cpl_recipe* _recipe = (cpl_recipe*)plugin;

                /* Ignore a failure here */
                (void)cpl_dfs_update_product_header(_recipe->frames);

            }
        }
        else {
            status = 0;
        }

        cpl_errorstate_set(inistate); /* Recover from recipe errors */
    }
    else {
        status = -1;
    }


    /* Release array elements. */
    if (plugin) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) plugin, 0);
    }

    /* Return the execution status. */
    return status;

}

JNIEXPORT jint JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeExecute(JNIEnv *env, jobject this,
                                             jstring jCurrentDir,
                                             jstring jExecDir)
{

    const char *currentDir;
    const char *execDir;

    int status = 0;
    int ok = 1;
    int errnum = 0;
    int dirChanged = 0;


    /*
     * Try to change directory to execution directory if one is defined.
     */

    if (jExecDir) {
        execDir = (*env)->GetStringUTFChars(env, jExecDir, NULL);
        dirChanged = !chdir(execDir);
        ok = ok && dirChanged;
        if (!ok) {
            errnum = errno;
        }
        (*env)->ReleaseStringUTFChars(env, jExecDir, execDir);
    }


    /*
     * Execute the recipe.
     */

    if (ok) {
        status = doExec(env, this);
    }

    /*
     * Try to change back to the orginal directory if we changed before.
     */

    if (dirChanged) {
        currentDir = (*env)->GetStringUTFChars(env, jCurrentDir, NULL);
        dirChanged = !chdir(currentDir);
        ok = ok && dirChanged;
        if (!dirChanged) {
            errnum = errno;
        }
        (*env)->ReleaseStringUTFChars(env, jCurrentDir, currentDir);
    }

    if (errnum) {
        (*env)->ThrowNew(env, CPLExceptionClass, strerror(errnum));
    }

    return status;

}


JNIEXPORT jobjectArray JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeGetParameterArray(JNIEnv *env,
                                                       jobject this)
{

    int ok = 1;

    cpl_recipe *recipe = NULL;
    cpl_parameterlist *parlist = NULL;

    jbyteArray jState = NULL;


    /*
     * Get the pointer to the parameter list.
     */

    ok = ok &&
            (jState = (*env)->GetObjectField(env, this, JNIRecipeStateField)) &&
            (recipe = (cpl_recipe *)
            (*env)->GetByteArrayElements(env, jState, NULL)) &&
            (parlist = recipe->parameters);


    /*
     * Release array elements.
     */

    if (recipe) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) recipe, 0);
    }


    /*
     * Return an array of Parameter objects based on the parlist.
     */

    return ok ? makeParameterArray(env, parlist, this) : NULL;

}


JNIEXPORT jobjectArray JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeGetFrameArray(JNIEnv *env, jobject this)
{
    int ok = 1;
    cpl_recipe *recipe = NULL;
    cpl_frameset *fset = NULL;

    jbyteArray jState = NULL;

    jobjectArray rc;


    /*
     * Get the pointer to the frameset.
     */

    ok = ok &&
            (jState = (*env)->GetObjectField(env, this, JNIRecipeStateField)) &&
            (recipe = (cpl_recipe *)
            (*env)->GetByteArrayElements(env, jState, NULL)) &&
            (fset = recipe->frames);


    /*
     * Release array elements.
     */

    if (recipe) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) recipe, 0);
    }

    /*
     * Return an array of Frame objects based on the frameset.
     */

    rc = ok ? makeFrameArray(env, fset) : NULL;

    return rc;

}


JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_JNIRecipe_nativeSetFrameArray(JNIEnv *env, jobject this,
                                                   jobjectArray jFrames)
{

    int ok = 1;

    cpl_recipe *recipe = NULL;

    cpl_frameset *fset = NULL;

    jbyteArray jState = NULL;


    /*
     * Create a new frameset based on the frames we have been given.
     */

    ok = ok && (fset = makeFrameSet(env, jFrames));


    /*
     * Get the pointer to the recipe structure.
     */

    ok = ok &&
            (jState = (*env)->GetObjectField(env, this,
                                             JNIRecipeStateField)) &&
            (recipe = (cpl_recipe *) (*env)->GetByteArrayElements(env, jState,
                                                                  NULL));

    /*
     * If a frameset exists there, destroy it.
     */

    if (ok && recipe->frames) {
        cpl_frameset_delete(recipe->frames);
    }


    /*
     * And write the new frameset in.
     */

    if (ok) {
        recipe->frames = fset;
    }


    /*
     * Release resources.
     */

    if (recipe) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) recipe, 0);
    }

    return;

}


/*
 * JNIParameterImp native methods.
 */

JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_JNIParameterImp_nativeSetValue(JNIEnv *env, jobject this,
                                                    jobject jVal)
{

    const char *strVal;

    int boolVal;
    int intVal;
    int ok = 1;

    double doubleVal;

    cpl_parameter **param_p = (cpl_parameter **)0;
    cpl_parameter *param = (cpl_parameter *)0;

    jbyteArray jState;


    /*
     * Get the parameter structure that this object refers to.
     */

    ok = ok &&
            (jState = (*env)->GetObjectField(env, this,
                                             JNIParameterImpStateField)) &&
            (param_p = (cpl_parameter **)
             (*env)->GetByteArrayElements(env, jState, NULL)) &&
            (param = *param_p);

    if (param_p) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) param_p, 0);
    }


    /*
     * Set the value.
     */

    switch (cpl_parameter_get_type(param)) {
        case CPL_TYPE_BOOL:
            boolVal = (*env)->CallBooleanMethod(env, jVal,
            BooleanBooleanValueMethod);
            ok = ok &&
                    !(*env)->ExceptionCheck(env) &&
                    !cpl_parameter_set_bool(param, boolVal);
            break;

        case CPL_TYPE_INT:
            intVal = (*env)->CallIntMethod(env, jVal,
            IntegerIntValueMethod);
            ok = ok &&
                    !(*env)->ExceptionCheck(env) &&
                    !cpl_parameter_set_int(param, intVal);
            break;

        case CPL_TYPE_DOUBLE:
            doubleVal = (*env)->CallDoubleMethod(env, jVal,
            DoubleDoubleValueMethod);
            ok = ok &&
                    !(*env)->ExceptionCheck(env) &&
                    !cpl_parameter_set_double(param, doubleVal);
            break;

        case CPL_TYPE_STRING:
            strVal = NULL;
            ok = ok &&
                    (strVal = (*env)->GetStringUTFChars(env, (jstring) jVal, NULL)) &&
                    !cpl_parameter_set_string(param, strVal);

            if (strVal) {
                (*env)->ReleaseStringUTFChars(env, (jstring) jVal, strVal);
            }
            break;

        default:
            (*env)->ThrowNew(env, IllegalStateExceptionClass,
                             "Unknown parameter type" );
            return;
    }


   /*
    * An error shouldn't happen here, since validation before this stage
    * should have caught bad values, but better check.
    */

    if (!ok && !(*env)->ExceptionCheck(env)) {
        (*env)->ThrowNew(env, IllegalArgumentExceptionClass,
                         "Error setting value");
    }

    return;

}


/**********************************************************************
 * PluginLibrary native methods.
 **********************************************************************/

JNIEXPORT void JNICALL
Java_org_eso_cpl_jni_PluginLibrary_nativeUnload(JNIEnv *env, jobject this)
{

    jobject jState = NULL;
    lt_dlhandle *handle_p = NULL;
    int ok = 1;
    jthrowable error = NULL;

    ok = ok &&
            (jState = (*env)->GetObjectField(env, this, PluginLibraryStateField)) &&
            (handle_p = (lt_dlhandle *)
            (*env)->GetByteArrayElements(env, jState, NULL));
    if (ok) {
        if (!lt_dlclose(*handle_p)) {
            error = makeLTDLException(env);
        }
    }
    if (handle_p) {
        (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) handle_p, 0);
    }
    if (error) {
        (*env)->Throw(env, error);
    }

    return;

}


/*
 * NativeTest native methods.
 */

#if defined (HAVE_ORG_ESO_CPL_TEST_NATIVE_TEST_H) && \
    defined (ENABLE_NATIVE_TESTS)

JNIEXPORT void JNICALL
Java_org_eso_cpl_test_NativeTest_nativeLTDLTest(JNIEnv *env, jclass class)
{
    const char *lname = "gimasterbias.so";

    lt_dlhandle dlhandle;

    lt_dlinit();
    dlhandle = lt_dlopen(lname);

    /*
    * FIXME: It's not found because the path is wrong - this
    *        doesn't prove much.
    */

    lt_dlexit();
}

#endif


/**********************************************************************
 * Static functions for determining compile-time constant values.
 **********************************************************************/

#define TEST_ID_NAME(Xname) \
   if (!strcmp(idname, #Xname)) { \
      id = Xname; \
      done = 1; \
}


static cpl_frame_group getFrameGroupID(JNIEnv *env, jobject jFgroup) {
    cpl_frame_group id = CPL_FRAME_GROUP_NONE;
    jstring jIdname = (jstring)0;
    const char *idname = (char *)0;
    int done = 0;
    int ok = 1;

    ok = ok &&
            jFgroup &&
            (!(*env)->ExceptionCheck(env)) &&
            (jIdname = (*env)->GetObjectField(env, jFgroup, FrameGroupIDNameField)) &&
            (idname = (*env)->GetStringUTFChars(env, jIdname, NULL));
    if (ok) {
        TEST_ID_NAME(CPL_FRAME_GROUP_NONE)
                else TEST_ID_NAME(CPL_FRAME_GROUP_RAW)
                else TEST_ID_NAME(CPL_FRAME_GROUP_CALIB)
                else TEST_ID_NAME(CPL_FRAME_GROUP_PRODUCT)
                (*env)->ReleaseStringUTFChars(env, jIdname, idname);
    }
    if (!done && !(*env)->ExceptionCheck(env)) {
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame group ID");
    }
    return id;
}


static cpl_frame_level getFrameLevelID(JNIEnv *env, jobject jFlevel) {
    cpl_frame_level id = CPL_FRAME_LEVEL_NONE;
    jstring jIdname = (jstring)0;
    const char *idname = (char *)0;
    int done = 0;
    int ok = 1;

    ok = ok &&
            jFlevel &&
            (!(*env)->ExceptionCheck(env)) &&
            (jIdname = (*env)->GetObjectField(env, jFlevel, FrameLevelIDNameField)) &&
            (idname = (*env)->GetStringUTFChars(env, jIdname, NULL));
    if (ok) {
        TEST_ID_NAME(CPL_FRAME_LEVEL_NONE)
                else TEST_ID_NAME(CPL_FRAME_LEVEL_TEMPORARY)
                else TEST_ID_NAME(CPL_FRAME_LEVEL_INTERMEDIATE)
                else TEST_ID_NAME(CPL_FRAME_LEVEL_FINAL)
                (*env)->ReleaseStringUTFChars(env, jIdname, idname);
    }
    if (!done && (*env)->ExceptionCheck(env)) {
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame level ID");
    }
    return id;
}


static cpl_frame_type getFrameTypeID(JNIEnv *env, jobject jFtype) {
    cpl_frame_type id = CPL_FRAME_TYPE_NONE;
    jstring jIdname = (jstring)0;
    const char *idname = (char *)0;
    int done = 0;
    int ok = 1;

    ok = ok &&
            jFtype &&
            (!(*env)->ExceptionCheck(env)) &&
            (jIdname = (*env)->GetObjectField(env, jFtype, FrameTypeIDNameField)) &&
            (idname = (*env)->GetStringUTFChars(env, jIdname, NULL));
    if (ok) {
        TEST_ID_NAME(CPL_FRAME_TYPE_NONE)
                else TEST_ID_NAME(CPL_FRAME_TYPE_IMAGE)
                else TEST_ID_NAME(CPL_FRAME_TYPE_MATRIX)
                else TEST_ID_NAME(CPL_FRAME_TYPE_TABLE)
    }
    if (!done && (*env)->ExceptionCheck(env)) {
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame type ID");
    }
    return id;
}

#undef TEST_ID_NAME


/**********************************************************************
 * Static functions.
 **********************************************************************/

/* Define some macros for doing the reflection which provides some of
 * the static variables required here.  Each of these macros returns
 * true (non-zero) only if the reflection has gone according to plan;
 * if a false (zero) value is returned then something has gone wrong,
 * and no further JNI methods should be called since an exception is
 * probably pending. */
#define DEFINE_CLASS(Xname, Xpath) \
         (Xname = (*env)->NewGlobalRef(env, (*env)->FindClass(env, Xpath)))
#define DEFINE_CONSTRUCTOR(Xname, Xclass, Xargs) \
         (Xname = (*env)->GetMethodID(env, Xclass, "<init>", "(" Xargs ")V"))
#define DEFINE_METHOD(Xname, Xclass, Xmethod, Xsig) \
      (Xname = (*env)->GetMethodID(env, Xclass, Xmethod, Xsig))
#define DEFINE_STATICMETHOD(Xname, Xclass, Xmethod, Xsig) \
      (Xname = (*env)->GetStaticMethodID(env, Xclass, Xmethod, Xsig))
#define DEFINE_FIELD(Xname, Xclass, Xfield, Xsig) \
      (Xname = (*env)->GetFieldID(env, Xclass, Xfield, Xsig))
#define DEFINE_CONSTANT(Xname, Xclass, Xfield, Xsig) \
      ((constField = (*env)->GetStaticFieldID(env, Xclass, Xfield, Xsig)) && \
       (Xname = (*env)->NewGlobalRef(env, \
                   (*env)->GetStaticObjectField(env, Xclass, constField))))

/*
 * Lazily performs initialization of some static variables required here.
 * If the initialization succeeds, a non-zero value is returned.
 * If it fails, there is a zero return and an exception is thrown.
 */

static int ensureConstantsSetup(JNIEnv *env)
{
   jfieldID constField;
   if (!isConstantsSetup) {
      int ok = 1;

      /* Do the reflection. */
      ok = ok &&
      DEFINE_CLASS(AssertionErrorClass, "java/lang/AssertionError") &&
      DEFINE_CLASS(BooleanClass, "java/lang/Boolean") &&
      DEFINE_CLASS(DoubleClass, "java/lang/Double") &&
      DEFINE_CLASS(FileClass, "java/io/File") &&
      DEFINE_CLASS(IllegalArgumentExceptionClass,
                   "java/lang/IllegalArgumentException") &&
      DEFINE_CLASS(IllegalStateExceptionClass,
                   "java/lang/IllegalStateException") &&
      DEFINE_CLASS(IntegerClass, "java/lang/Integer") &&
      DEFINE_CLASS(LinkageErrorClass, "java/lang/LinkageError") &&
      DEFINE_CLASS(NullPointerExceptionClass,
                   "java/lang/NullPointerException") &&
      DEFINE_CLASS(ObjectClass, "java/lang/Object") &&
      DEFINE_CLASS(PrintStreamClass, "java/io/PrintStream") &&
      DEFINE_CLASS(RuntimeExceptionClass, "java/lang/RuntimeException") &&
      DEFINE_CLASS(StringClass, "java/lang/String") &&

      DEFINE_CLASS(BadAPIPluginClass,
                   "org/eso/cpl/jni/OtherPlugin$BadAPIPlugin") &&
      DEFINE_CLASS(CPLControlClass,
                   "org/eso/cpl/jni/CPLControl") &&
      DEFINE_CLASS(CPLExceptionClass, "org/eso/cpl/CPLException") &&
      DEFINE_CLASS(EnumConstraintClass, "org/eso/cpl/EnumConstraint") &&
      DEFINE_CLASS(FrameClass, "org/eso/cpl/Frame") &&
      DEFINE_CLASS(FrameGroupClass, "org/eso/cpl/FrameGroup") &&
      DEFINE_CLASS(FrameLevelClass, "org/eso/cpl/FrameLevel") &&
      DEFINE_CLASS(FrameTypeClass, "org/eso/cpl/FrameType") &&
      DEFINE_CLASS(JNIParameterImpClass, "org/eso/cpl/jni/JNIParameterImp") &&
      DEFINE_CLASS(JNIRecipeClass, "org/eso/cpl/jni/JNIRecipe") &&
      DEFINE_CLASS(LTDLExceptionClass, "org/eso/cpl/jni/LTDLException") &&
      DEFINE_CLASS(NonRecipePluginClass,
                   "org/eso/cpl/jni/OtherPlugin$NonRecipePlugin") &&
      DEFINE_CLASS(OtherPluginClass, "org/eso/cpl/jni/OtherPlugin") &&
      DEFINE_CLASS(ParameterClass, "org/eso/cpl/Parameter") &&
      DEFINE_CLASS(ParameterTypeClass, "org/eso/cpl/ParameterType") &&
      DEFINE_CLASS(ParameterValueExceptionClass,
                   "org/eso/cpl/ParameterValueException") &&
      DEFINE_CLASS(PluginLibraryClass, "org/eso/cpl/jni/PluginLibrary") &&
      DEFINE_CLASS(RangeConstraintClass, "org/eso/cpl/RangeConstraint") &&

      DEFINE_CONSTRUCTOR(BadAPIPluginConstructor, BadAPIPluginClass,
                         "Ljava/lang/String;II") &&
      DEFINE_CONSTRUCTOR(DoubleConstructor, DoubleClass, "D") &&
      DEFINE_CONSTRUCTOR(EnumConstraintConstructor, EnumConstraintClass,
                         "[Ljava/lang/Object;") &&
      DEFINE_CONSTRUCTOR(FileConstructor, FileClass, "Ljava/lang/String;") &&
      DEFINE_CONSTRUCTOR(FrameConstructor, FrameClass, "Ljava/io/File;") &&
      DEFINE_CONSTRUCTOR(IntegerConstructor, IntegerClass, "I") &&
      DEFINE_CONSTRUCTOR(JNIParameterImpConstructor, JNIParameterImpClass,
                         "[BLorg/eso/cpl/jni/JNIRecipe;"
                         "Lorg/eso/cpl/ParameterType;"
                         "Lorg/eso/cpl/ParameterConstraint;"
                         "Ljava/lang/Object;") &&
      DEFINE_CONSTRUCTOR(JNIRecipeConstructor, JNIRecipeClass,
                         "[BLorg/eso/cpl/jni/PluginLibrary;") &&
      DEFINE_CONSTRUCTOR(LTDLExceptionConstructor, LTDLExceptionClass,
                         "Ljava/lang/String;") &&
      DEFINE_CONSTRUCTOR(NonRecipePluginConstructor, NonRecipePluginClass,
                         "Ljava/lang/String;") &&
      DEFINE_CONSTRUCTOR(ParameterConstructor, ParameterClass,
                         "Lorg/eso/cpl/ParameterImp;") &&
      DEFINE_CONSTRUCTOR(PluginLibraryConstructor, PluginLibraryClass,
                         "[BLjava/lang/String;[Lorg/eso/cpl/jni/JNIRecipe;"
                         "[Lorg/eso/cpl/jni/OtherPlugin;") &&
      DEFINE_CONSTRUCTOR(RangeConstraintConstructor, RangeConstraintClass,
                         "Lorg/eso/cpl/ParameterType;"
                         "Ljava/lang/Object;Ljava/lang/Object;") &&

      DEFINE_METHOD(BooleanBooleanValueMethod, BooleanClass,
                    "booleanValue", "()Z") &&
      DEFINE_METHOD(DoubleDoubleValueMethod, DoubleClass,
                    "doubleValue", "()D") &&
      DEFINE_METHOD(IntegerIntValueMethod, IntegerClass,
                    "intValue", "()I") &&
      DEFINE_METHOD(PrintStreamPrintlnMethod, PrintStreamClass,
                    "println", "(Ljava/lang/String;)V") &&

      DEFINE_STATICMETHOD(CPLControlErrMessageMethod, CPLControlClass,
                          "errMessage", "(Ljava/lang/String;)V") &&
      DEFINE_STATICMETHOD(CPLControlLogMessageMethod, CPLControlClass,
                          "logMessage",
                          "(Ljava/lang/String;Ljava/lang/String;I)V") &&
      DEFINE_STATICMETHOD(CPLControlOutMessageMethod, CPLControlClass,
                          "outMessage", "(Ljava/lang/String;)V") &&

      DEFINE_FIELD(FrameCanonicalPathField, FrameClass,
                   "canonicalPath_", "Ljava/lang/String;") &&
      DEFINE_FIELD(FrameGroupField, FrameClass,
                   "group_", "Lorg/eso/cpl/FrameGroup;") &&
      DEFINE_FIELD(FrameLevelField, FrameClass,
                   "level_", "Lorg/eso/cpl/FrameLevel;") &&
      DEFINE_FIELD(FrameTagField, FrameClass,
                   "tag_", "Ljava/lang/String;") &&
      DEFINE_FIELD(FrameTypeField, FrameClass,
                   "type_", "Lorg/eso/cpl/FrameType;") &&
      DEFINE_FIELD(FrameGroupIDNameField, FrameGroupClass,
                   "idName_", "Ljava/lang/String;") &&
      DEFINE_FIELD(FrameLevelIDNameField, FrameLevelClass,
                   "idName_", "Ljava/lang/String;") &&
      DEFINE_FIELD(FrameTypeIDNameField, FrameTypeClass,
                   "idName_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIParameterImpStateField, JNIParameterImpClass,
                   "nativeState_", "[B") &&
      DEFINE_FIELD(JNIParameterImpContextField, JNIParameterImpClass,
                   "context_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIParameterImpHelpField, JNIParameterImpClass,
                   "help_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIParameterImpNameField, JNIParameterImpClass,
                   "name_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIParameterImpTagField, JNIParameterImpClass,
                   "tag_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeAuthorField, JNIRecipeClass,
                   "author_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeCopyrightField, JNIRecipeClass,
                   "copyright_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeDescriptionField, JNIRecipeClass,
                   "description_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeEmailField, JNIRecipeClass,
                   "email_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeNameField, JNIRecipeClass,
                   "name_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeStateField, JNIRecipeClass,
                   "nativeState_", "[B") &&
      DEFINE_FIELD(JNIRecipeSynopsisField, JNIRecipeClass,
                   "synopsis_", "Ljava/lang/String;") &&
      DEFINE_FIELD(JNIRecipeVersionField, JNIRecipeClass,
                   "version_", "J") &&
      DEFINE_FIELD(PluginLibraryStateField, PluginLibraryClass,
                   "nativeState_", "[B") &&

      DEFINE_CONSTANT(BooleanFALSEConstant, BooleanClass,
                      "FALSE", "Ljava/lang/Boolean;") &&
      DEFINE_CONSTANT(BooleanTRUEConstant, BooleanClass,
                      "TRUE", "Ljava/lang/Boolean;") &&
      DEFINE_CONSTANT(FrameGroupRAWConstant, FrameGroupClass,
                      "RAW", "Lorg/eso/cpl/FrameGroup;") &&
      DEFINE_CONSTANT(FrameGroupCALIBConstant, FrameGroupClass,
                      "CALIB", "Lorg/eso/cpl/FrameGroup;") &&
      DEFINE_CONSTANT(FrameGroupPRODUCTConstant, FrameGroupClass,
                      "PRODUCT", "Lorg/eso/cpl/FrameGroup;") &&
      DEFINE_CONSTANT(FrameLevelTEMPORARYConstant, FrameLevelClass,
                      "TEMPORARY", "Lorg/eso/cpl/FrameLevel;") &&
      DEFINE_CONSTANT(FrameLevelINTERMEDIATEConstant, FrameLevelClass,
                      "INTERMEDIATE", "Lorg/eso/cpl/FrameLevel;") &&
      DEFINE_CONSTANT(FrameLevelFINALConstant, FrameLevelClass,
                      "FINAL", "Lorg/eso/cpl/FrameLevel;") &&
      DEFINE_CONSTANT(FrameTypeIMAGEConstant, FrameTypeClass,
                      "IMAGE", "Lorg/eso/cpl/FrameType;") &&
      DEFINE_CONSTANT(FrameTypeMATRIXConstant, FrameTypeClass,
                      "MATRIX", "Lorg/eso/cpl/FrameType;") &&
      DEFINE_CONSTANT(FrameTypeTABLEConstant, FrameTypeClass,
                      "TABLE", "Lorg/eso/cpl/FrameType;") &&
      DEFINE_CONSTANT(ParameterTypeBOOLConstant, ParameterTypeClass,
                      "BOOL", "Lorg/eso/cpl/ParameterType;") &&
      DEFINE_CONSTANT(ParameterTypeDOUBLEConstant, ParameterTypeClass,
                      "DOUBLE", "Lorg/eso/cpl/ParameterType;") &&
      DEFINE_CONSTANT(ParameterTypeINTConstant, ParameterTypeClass,
                      "INT", "Lorg/eso/cpl/ParameterType;") &&
      DEFINE_CONSTANT(ParameterTypeSTRINGConstant, ParameterTypeClass,
                      "STRING", "Lorg/eso/cpl/ParameterType;") &&

      (VersionString = (*env)->NewGlobalRef(env,
         (*env)->NewStringUTF(env, JAVACPL_VERSION))) &&

      1;

      /* If there was a failure but we don't have an exception to show
       * for it, throw an exception here. */
      if (!ok && !(*env)->ExceptionOccurred(env)) {
         (*env)->ThrowNew(env, RuntimeExceptionClass, "Reflection trouble");
      }
      isConstantsSetup = ok;
   }
   return isConstantsSetup;
}

#undef DEFINE_CLASS
#undef DEFINE_CONSTRUCTOR
#undef DEFINE_METHOD
#undef DEFINE_STATICMETHOD
#undef DEFINE_FIELD
#undef DEFINE_CONSTANT


static int ensureHandlersSetup(JNIEnv *env) {
   if (!isHandlersSetup) {
      int ok = 1;

      cpl_init(CPL_INIT_DEFAULT);

      if (cpl_error_get_code()) {
          cpl_msg_warning("Java CPL", "CPL initialization failed: '%s' at %s\n",
                          cpl_error_get_message(), cpl_error_get_where());
      } else {
          (void)setCplMessaging(CPL_TRUE);
      }

#ifndef GASGANO_HAS_NO_CPL_EXCEPTION_HANDLER
      if (cpl_error_get_code() && !(*env)->ExceptionCheck(env)) {
          /* The only option for handling the error at this point is to throw
              an exception. */
          (*env)->ThrowNew(env, CPLExceptionClass, "CPL initialization failed");
      }
#endif
      inistate = cpl_errorstate_get();
      isHandlersSetup = ok;
   }
   return isHandlersSetup;
}


/*
 * Sets up the stdout, stderr and default logging handlers that are used by
 * cext's message handling facilities.
 */

static cpl_error_code setCplMessaging(cpl_boolean startlog) {

    cpl_errorstate prestate = cpl_errorstate_get();

    /* cx_log_level_flags cx_level = CX_LOG_LEVEL_DEBUG; */
    /* cpl_msg_severity cpl_level = CPL_MSG_DEBUG; */
    cpl_msg_severity cpl_level = CPL_MSG_INFO;
    cpl_msg_severity cpl_log_level = CPL_MSG_INFO;
    cpl_msg_set_domain("JavaCPL");
    cpl_msg_set_domain_on();
    cx_print_set_handler(out_message_handler);
    cx_printerr_set_handler(err_message_handler);
    cx_log_set_default_handler(default_log_message_handler);
    cpl_msg_set_time_on();
    cpl_msg_set_component_off();
    cpl_msg_set_domain_off();
    cpl_msg_set_width(132);
    cpl_msg_set_indentation(4);
    cpl_msg_set_level(cpl_level);
    if (startlog) cpl_msg_set_log_level(cpl_log_level); /* Do this only once */

    if (!cpl_errorstate_is_equal(prestate)) {

        cpl_msg_warning("Java CPL", "CPL-messaging initialization failed:\n");

        cpl_errorstate_dump(prestate, CPL_FALSE, NULL);

        return cpl_error_set_where(cpl_func);
    } else {
        return CPL_ERROR_NONE;
    }
}

static void out_message_handler(const cxchar *msg) {
   JNIEnv *env;
   jstring jMsg = (jstring)0;
   int ok = 1;
   /* java method cannot be called from multiple threads
    * for unknown reasons a lock is insufficient too */
#ifdef _OPENMP
   ok = omp_get_thread_num() == 0;
#endif

   ok = ok &&
      (env = currentEnv);
   if (msg) {
      ok = ok &&
         (jMsg = (*env)->NewStringUTF(env, msg));
   }
   if (ok) {
      (*env)->CallStaticVoidMethod(env, CPLControlClass,
                                   CPLControlOutMessageMethod, jMsg);
   }
   else {
      fprintf(stdout, "Lost output message: %s\n", msg ? msg : "null");
   }
}


static void err_message_handler(const cxchar *msg) {
   JNIEnv *env;
   jstring jMsg = (jstring)0;
   int ok = 1;
   /* java method cannot be called from multiple threads
    * for unknown reasons a lock is insufficient too */
#ifdef _OPENMP
   ok = omp_get_thread_num() == 0;
#endif

   ok = ok &&
      (env = currentEnv);
   if (msg) {
      ok = ok &&
         (jMsg = (*env)->NewStringUTF(env, msg));
   }
   if (ok) {
      (*env)->CallStaticVoidMethod(env, CPLControlClass,
                                   CPLControlErrMessageMethod, jMsg);
   }
   else {
      fprintf(stderr, "Lost error message: %s\n", msg ? msg : "null");
   }
}


static void default_log_message_handler(const cxchar *domain,
                                        cx_log_level_flags flags,
                                        const cxchar *msg, cxptr ptr) {
   JNIEnv *env = currentEnv;
   jstring jDomain;
   jstring jMsg;
   int ok = 1;
   /* java method cannot be called from multiple threads
    * for unknown reasons a lock is insufficient too */
#ifdef _OPENMP
   ok = omp_get_thread_num() == 0;
#endif

   ok = ok && env;
   jMsg = NULL;
   if (msg) {
      ok = ok &&
         (jMsg = (*env)->NewStringUTF(env, msg));
   }
   jDomain = NULL;
   if (domain) {
      ok = ok &&
         (jDomain = (*env)->NewStringUTF(env, domain));
   }
   if (ok) {
      (*env)->CallStaticVoidMethod(env, CPLControlClass,
                                   CPLControlLogMessageMethod, jDomain, jMsg,
                                   (jint) flags);
   }
   else {
      /* Use ptr to avoid compiler warning */
      char * nullmsg = cpl_sprintf("null (ptr=%p)", (const void*)ptr);
      fprintf(stderr, "Lost log message: %s\n", msg ? msg : nullmsg);
      cpl_free(nullmsg);
   }
}


/*
 * Returns an LTDLException with a message taken from LTDL's record of
 * the most recent error to occur.
 */

static jthrowable makeLTDLException(JNIEnv *env) {
   const char *msg;
   jstring jMsg;
   jthrowable error = (jthrowable)0;

   /* Check that some constants have been initialized.  If they have not
    * for some reason, throw the exception which describes why not. */
   if (!ensureConstantsSetup(env)) {
      error = (*env)->ExceptionOccurred(env);
      (*env)->ExceptionClear(env);
      return error;
   }

   /* Get the error message. */
   msg = lt_dlerror();
   if (!msg) {
      msg = "no error";
   }
   jMsg = (*env)->NewStringUTF(env, msg);
   if (jMsg) {
      error = (*env)->NewObject(env, LTDLExceptionClass,
                                LTDLExceptionConstructor, jMsg);
   }

   /* If for some reason construction of the exception failed, return the
    * reason for its failure, and clear the exception. */
   if (!error) {
      error = (*env)->ExceptionOccurred(env);
      (*env)->ExceptionClear(env);
   }

   /* Return the exception. */
   return error;
}


/*
 * Constructs and returns a PluginLibrary object from a DLL handle.
 */

static jobject makePluginLibrary(JNIEnv *env, jstring jLocation,
                                 lt_dlhandle dlhandle) {
    jobject jPluglib = (jobject)0;

#ifdef GASGANO_HAS_NO_CPL_EXCEPTION_HANDLER
    (void)setCplMessaging(CPL_FALSE);
    inistate = cpl_errorstate_get();
#else
    if (cpl_error_get_code() || setCplMessaging(CPL_FALSE) != CPL_ERROR_NONE) {
        cpl_msg_warning("Java CPL", "Not loading plugins due to pre-existing "
                        "CPL error\n");
    } else
#endif
    if (1) {

        const char * cpl_version;
        int (*list_function)(cpl_pluginlist *);
        const cpl_plugin *plugin;
        cpl_pluginlist *pluginlist;
        jsize nplugin;
        jsize nrec;
        jsize nother;
        jsize irec;
        jsize iother;
        jobjectArray jRecipes = (jobjectArray)0;
        jobjectArray jOthers = (jobjectArray)0;
        jbyteArray jState = (jobjectArray)0;
        jbyte *state = NULL;
        int ok = 1;

        /* Get the listing function from the DLL. The listing function
         * is declared like this in cpl_plugininfo.h:
         *
         *    int cpl_plugin_get_info(cpl_pluginlist *);
         */

        /* Cast from object pointer to function pointer is not allowed
           in ISO C. Circumvent the cast with a union... */
        union cast_dl_ {
            void * address;
            int (*function)(cpl_pluginlist *);
        } cast_dl;

        if (sizeof(cast_dl.address) != sizeof(cast_dl.function)) {
            /* Something serious is wrong, bail out */
#ifndef GASGANO_HAS_NO_CPL_EXCEPTION_HANDLER
            (*env)->Throw(env, makeLTDLException(env));
#endif
            return jPluglib;
        }

        cast_dl.address = (void*)lt_dlsym(dlhandle, "cpl_plugin_get_info");
        list_function = cast_dl.function;

        if (list_function == NULL) {
            /* The listing function is not available */
#ifndef GASGANO_HAS_NO_CPL_EXCEPTION_HANDLER
            (*env)->Throw(env, makeLTDLException(env));
#endif
            return jPluglib;
        }

        cpl_version = cpl_version_get_version();
        if (cpl_version != NULL &&
            strcmp(PACKAGE_VERSION, cpl_version)) {
            /* The plugin has caused functions from libcplcore.so that have not
               already been loaded, to be loaded from the wrong libcplcore.so,
               f.ex. the libcplcore.so used to compile the plugin */
#ifndef GASGANO_HAS_NO_CPL_EXCEPTION_HANDLER
            (*env)->ThrowNew(env, CPLExceptionClass, "CPL " PACKAGE_VERSION
                             " library version error");
#endif
            return jPluglib;
        }

        /* Populate a plugin list with all the plugins known by this DLL.
         * list_function returns an int, zero on success and one on failure. */
        pluginlist = cpl_pluginlist_new();
        ok = ok && !( (*list_function)(pluginlist) );

        /* Split the list of plugins into two mutually exclusive sublists;
         * usable recipes and the rest. */
        nplugin = cpl_pluginlist_get_size(pluginlist);

        /* Count the number of recipes */
        nrec = 0;
        for (plugin = cpl_pluginlist_get_first(pluginlist); plugin;
             plugin = cpl_pluginlist_get_next(pluginlist)) {
            const unsigned int papi = cpl_plugin_get_api(plugin);
            const cpl_plugin_type ptype = cpl_plugin_get_type(plugin);

            if ((papi == CPL_PLUGIN_API) && (ptype & CPL_PLUGIN_TYPE_RECIPE)) {
                nrec++;
            }
        }
        nother = nplugin - nrec;

        /* Construct a new PluginLibrary object which will contain the recipes. */
        ok = ok &&
            (jState = (*env)->NewByteArray(env, sizeof(lt_dlhandle))) &&
            (state = (*env)->GetByteArrayElements(env, jState, NULL)) &&
            (jRecipes = (*env)->NewObjectArray(env, nrec, JNIRecipeClass, NULL)) &&
            (jOthers = (*env)->NewObjectArray(env, nother, OtherPluginClass, NULL)) &&
            (jPluglib = (*env)->NewObject(env, PluginLibraryClass,
                                          PluginLibraryConstructor, jState,
                                          jLocation, jRecipes, jOthers));
        if (state && ok) {
            *((lt_dlhandle *) state) = dlhandle;
            (*env)->ReleaseByteArrayElements(env, jState, state, 0);
        }

        /* Populate the array of recipe objects. */
        /* Populate the array of other plugins. */
        irec = 0;
        iother = 0;
        for (plugin = cpl_pluginlist_get_first(pluginlist); ok && plugin;
             plugin = cpl_pluginlist_get_next(pluginlist)) {
            const unsigned int papi = cpl_plugin_get_api(plugin);
            const cpl_plugin_type ptype = cpl_plugin_get_type(plugin);

            if ((papi == CPL_PLUGIN_API) && (ptype & CPL_PLUGIN_TYPE_RECIPE)) {
                const cpl_recipe *recipe = (const cpl_recipe *)plugin;
                jobject jRecipe = (jobject)0;
                ok = ok &&
                    (jRecipe = makeJNIRecipe(env, recipe, jPluglib));
                if (ok) {
                    (*env)->SetObjectArrayElement(env, jRecipes, irec, jRecipe);
                }
                irec++;
            } else {
                jstring jName;
                jobject jOther = (jobject)0;
                const char *name = cpl_plugin_get_name(plugin);

                /* If we were truly paranoid we wouldn't attempt to read the name
                 * unless we know the API versions match, but it's probably OK... */
                jName = (*env)->NewStringUTF(env, name);
                if (papi != CPL_PLUGIN_API) {
                    ok = ok &&
                        (jOther = (*env)->NewObject(env, BadAPIPluginClass,
                                                    BadAPIPluginConstructor,
                                                    jName,
                                                    (jint) CPL_PLUGIN_API,
                                                    (jint) papi));
                }
                else {
                    cx_assert((ptype & CPL_PLUGIN_TYPE_RECIPE) == 0);
                    ok = ok &&
                        (jOther = (*env)->NewObject(env, NonRecipePluginClass,
                                                    NonRecipePluginConstructor,
                                                    jName));
                }
                if (ok) {
                    (*env)->SetObjectArrayElement(env, jOthers, iother, jOther);
                }
                iother++;
            }
        }
        cx_assert( irec   == nrec   );
        cx_assert( iother == nother );
        cx_assert( nother + nrec == nplugin );

        /* Free workspace. */
        cpl_pluginlist_delete(pluginlist);
    }

    /* Return the library object. */
    return jPluglib;
}


/*
 * Turns a CPL frameset into an array of java Frame objects.
 */

static jobjectArray makeFrameArray(JNIEnv *env, cpl_frameset *fset) {
   jobject jFrame = (jobject)0;
   jobjectArray jFrames = (jobjectArray)0;
   cpl_frame *frame;
   jsize ifrm;
   jsize nfrm;
   int ok = 1;

   /* Determine the size of the frameset. */
   nfrm = cpl_frameset_get_size(fset);

   /* Construct an empty array to hold the new Frame objects. */
   ok = ok &&
      (jFrames = (*env)->NewObjectArray(env, nfrm, FrameClass, NULL));

   /* Populate the array with new Frame objects constructed from the frames
    * in the frameset. */
   ifrm = 0;

   cpl_frameset_iterator *it = cpl_frameset_iterator_new(fset);

   while (ok && (frame = cpl_frameset_iterator_get(it)))
   {
      ok = ok &&
         (jFrame = makeJavaFrame(env, frame));
      if (ok) {
         (*env)->SetObjectArrayElement(env, jFrames, ifrm++, jFrame);
      }

      cpl_frameset_iterator_advance(it, 1);
   }

   cpl_frameset_iterator_delete(it);

   /* Return the array. */
   return ok ? jFrames : NULL;
}


/*
 * Turns a CPL frame into a java Frame object.
 */

static jobject makeJavaFrame(JNIEnv *env, cpl_frame *frame) {
   jobject jFrame;
   const char *filename;
   const char *tag;
   jstring jFilename;
   jobject jFile;
   jstring jTag;
   jobject jFtype;
   jobject jFgroup;
   jobject jFlevel;
   int frameType;
   int frameGroup;
   int frameLevel;

   /* Get the frame's filename as a java string. */
   filename = cpl_frame_get_filename(frame);
   jFilename = filename ? (*env)->NewStringUTF(env, filename) : NULL;
   jFile = jFilename ? (*env)->NewObject(env, FileClass, FileConstructor,
                                         jFilename)
                     : NULL;

   /* Get the tag as a java string. */
   tag = cpl_frame_get_tag(frame);
   jTag = tag ? (*env)->NewStringUTF(env, tag) : NULL;

   /* Get the frame type. */
   jFtype = NULL;

   /* In order to handle the possible addition of "new" frame types without
      throwing an exception, the default case behaves the same as
      type NONE.
   */
   frameType = cpl_frame_get_type(frame);
   switch (frameType) {
      case CPL_FRAME_TYPE_NONE:
         jFtype = NULL;
         break;
      case CPL_FRAME_TYPE_IMAGE:
         jFtype = FrameTypeIMAGEConstant;
         break;
      case CPL_FRAME_TYPE_MATRIX:
         jFtype = FrameTypeMATRIXConstant;
         break;
      case CPL_FRAME_TYPE_TABLE:
         jFtype = FrameTypeTABLEConstant;
         break;
      default:
      /*
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame type");
        return NULL;
        map unexpected values to NULL
      */
         cpl_msg_warning("Java CPL", "Frame %s has unexpected frame type: %d\n",
                filename, frameType);
         jFtype = NULL;
   }

   /* Get the frame group. */
   jFgroup = NULL;

   /*
     In order to handle the possible addition of "new" frame groups without
     throwing an exception, the default case behaves the same as
     group NONE.
   */
   frameGroup = cpl_frame_get_group(frame);
   switch (frameGroup) {
      case CPL_FRAME_GROUP_NONE:
         jFgroup = NULL;
         break;
      case CPL_FRAME_GROUP_RAW:
         jFgroup = FrameGroupRAWConstant;
         break;
      case CPL_FRAME_GROUP_CALIB:
         jFgroup = FrameGroupCALIBConstant;
         break;
      case CPL_FRAME_GROUP_PRODUCT:
         jFgroup = FrameGroupPRODUCTConstant;
         break;
      default:
      /*
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame group");
        return NULL;
      */
         cpl_msg_warning("Java CPL", "Frame %s has unexpected frame group: %d\n",
                filename, frameGroup);
         jFgroup = NULL;
   }

   /* Get the frame level. */
   /*
     In order to handle the possible addition of "new" frame levels without
     throwing an exception, the default case behaves the same as
     level NONE.
   */
   jFlevel = NULL;
   frameLevel = cpl_frame_get_level(frame);
   switch (frameLevel) {
      case CPL_FRAME_LEVEL_NONE:
         jFlevel = NULL;
         break;
      case CPL_FRAME_LEVEL_TEMPORARY:
         jFlevel = FrameLevelTEMPORARYConstant;
         break;
      case CPL_FRAME_LEVEL_INTERMEDIATE:
         jFlevel = FrameLevelINTERMEDIATEConstant;
         break;
      case CPL_FRAME_LEVEL_FINAL:
         jFlevel = FrameLevelFINALConstant;
         break;
      default:
      /*
        (*env)->ThrowNew(env, RuntimeExceptionClass, "Unknown frame level");
        return NULL;
      */
         cpl_msg_warning("Java CPL", "Frame %s has unexpected frame level: %d\n",
                filename, frameLevel);
         jFlevel = NULL;
   }

   /* Construct a new Frame object and populate its fields with the
    * values we have calculated. */
   jFrame = (*env)->NewObject(env, FrameClass, FrameConstructor, jFile);
   (*env)->SetObjectField(env, jFrame, FrameTagField, jTag);
   (*env)->SetObjectField(env, jFrame, FrameTypeField, jFtype);
   (*env)->SetObjectField(env, jFrame, FrameGroupField, jFgroup);
   (*env)->SetObjectField(env, jFrame, FrameLevelField, jFlevel);

   /* Return the populated object. */
   return jFrame;
}


/*
 * Turns an array of java Frame objects into a CPL frameset.
 */

static cpl_frameset *makeFrameSet(JNIEnv *env, jobjectArray jFrames) {
   cpl_frameset *fset = (cpl_frameset *)0;
   cpl_frame *frame = (cpl_frame *)0;
   jobject jFrame = (jobject)0;
   jsize nfrm;
   jsize ifrm;
   int ok = 1;

   /* Get the size of the frame array. */
   nfrm = (*env)->GetArrayLength(env, jFrames);
   if ((*env)->ExceptionCheck(env)) {
      return NULL;
   }
   ok = ok &&
      (fset = cpl_frameset_new());
   for (ifrm = 0; ok && ifrm < nfrm; ifrm++) {
      ok = ok &&
         (jFrame = (*env)->GetObjectArrayElement(env, jFrames, ifrm)) &&
         (frame = makeCplFrame(env, jFrame)) &&
         (!cpl_frameset_insert(fset, frame));
      if (!jFrame && !(*env)->ExceptionCheck(env)) {
         (*env)->ThrowNew(env, NullPointerExceptionClass, NULL);
         return NULL;
      }
   }
   return ok ? fset : NULL;
}


static cpl_frame *makeCplFrame(JNIEnv *env, jobject jFrame) {
   cpl_frame *frame;
   jstring jFilename;
   jstring jTag;
   jobject jFtype;
   jobject jFgroup;
   jobject jFlevel;

   frame = cpl_frame_new();
   if (!frame) {
       return NULL;
   }
   jFilename = (*env)->GetObjectField(env, jFrame, FrameCanonicalPathField);
   if (jFilename) {
      const char *filename = (*env)->GetStringUTFChars(env, jFilename, NULL);
      cpl_frame_set_filename(frame, filename);
      (*env)->ReleaseStringUTFChars(env, jFilename, filename);
   }
   jTag = (*env)->GetObjectField(env, jFrame, FrameTagField);
   if (jTag) {
      const char *tag = (*env)->GetStringUTFChars(env, jTag, NULL);
      cpl_frame_set_tag(frame, tag);
      (*env)->ReleaseStringUTFChars(env, jTag, tag);
   }
   jFtype = (*env)->GetObjectField(env, jFrame, FrameTypeField);
   if (jFtype) {
      cpl_frame_set_type(frame, getFrameTypeID(env, jFtype));
   }
   jFgroup = (*env)->GetObjectField(env, jFrame, FrameGroupField);
   if (jFgroup) {
      cpl_frame_set_group(frame, getFrameGroupID(env, jFgroup));
   }
   jFlevel = (*env)->GetObjectField(env, jFrame, FrameLevelField);
   if (jFlevel) {
      cpl_frame_set_level(frame, getFrameLevelID(env, jFlevel));
   }
   return frame;
}


/*
 * Constructs and returns a new JNIRecipe object from a java byte[] array
 * which contains a cpl_recipe structure.  If it can't be done, returns
 * NULL.
 */

static jobject makeJNIRecipe(JNIEnv *env, const cpl_recipe *recipe,
                             jobject jPluglib) {
   int ok = 1;
   jbyteArray jState = NULL;
   cpl_recipe *state = NULL;
   jobject jRecipe = NULL;

   /* Copy the recipe struct into a java byte array. */
   ok = ok &&
      (jState = (*env)->NewByteArray(env, sizeof(cpl_recipe))) &&
      (state = (cpl_recipe *) (*env)->GetByteArrayElements(env, jState, NULL));
   if (ok) {
      *state = *recipe;
   }
   if (state) {
      (*env)->ReleaseByteArrayElements(env, jState, (jbyte *) state, 0);
   }

   /* Construct a new JNIRecipe object. */
   ok = ok &&
      (jRecipe = (*env)->NewObject(env, JNIRecipeClass, JNIRecipeConstructor,
                                   jState, jPluglib));

   /* Populate the JNIRecipe object's fields from the contents of the
    * cpl_recipe structure.
    * TODO: the struct members should not be used directly, instead
    * they should be accessed through functions of the form:
    * cpl_plugin_get_name()
    * cpl_plugin_get_version(), etc.
    */
   if (ok) {
       const cpl_plugin *plugin = (const cpl_plugin *) recipe;
      (*env)->SetObjectField(env, jRecipe, JNIRecipeAuthorField,
                             (*env)->NewStringUTF(env, plugin->author));
      (*env)->SetObjectField(env, jRecipe, JNIRecipeCopyrightField,
                             (*env)->NewStringUTF(env, plugin->copyright));
      (*env)->SetObjectField(env, jRecipe, JNIRecipeDescriptionField,
                             (*env)->NewStringUTF(env, plugin->description));
      (*env)->SetObjectField(env, jRecipe, JNIRecipeEmailField,
                             (*env)->NewStringUTF(env, plugin->email));
      (*env)->SetObjectField(env, jRecipe, JNIRecipeNameField,
                             (*env)->NewStringUTF(env, plugin->name));
      (*env)->SetObjectField(env, jRecipe, JNIRecipeSynopsisField,
                             (*env)->NewStringUTF(env, plugin->synopsis));
      (*env)->SetLongField(env, jRecipe, JNIRecipeVersionField,
                           (jlong) plugin->version);
   }

   /* Return the new object. */
   return ok ? jRecipe : NULL;
}


/*
 * Constructs and returns a new Parameter[] array built from the contents
 * of a cpl_parameterlist structure.
 */

static jobjectArray makeParameterArray(JNIEnv *env, cpl_parameterlist *parlist,
                                       jobject jRecipe) {
   int ok = 1;
   int npar;
   int ip;
   cpl_parameter *param;
   jobject jParam = (jobject)0;
   jobjectArray jParray = NULL;

   /* Null in, null out. */
   if (!parlist || (*env)->ExceptionCheck(env)) {
      return NULL;
   }

   /* Assemble a java array of all the cpl_parameters in this cpl_parameterlist. */
   ok = ok &&
      ((npar = cpl_parameterlist_get_size(parlist)) >= 0) &&
      (jParray = (*env)->NewObjectArray(env, npar, ParameterClass, NULL));
   ip = 0;
   for (param = cpl_parameterlist_get_first(parlist); ok && (param != NULL);
        param = cpl_parameterlist_get_next(parlist)) {
      ok = ok &&
         (jParam = makeParameter(env, param, jRecipe));
      if (ok) {
         (*env)->SetObjectArrayElement(env, jParray, ip++, jParam);
      }
   }

   /* Return the populated array. */
   return ok ? jParray : NULL;
}


/*
 * Constructs a Parameter object from a cpl_parameter in a cpl_parameterlist.
 */

static jobject makeParameter(JNIEnv *env, cpl_parameter *param,
                             jobject jRecipe) {
   cpl_type ptype;
   cpl_parameter_class pclass;
   jbyteArray jState = NULL;
   jbyte *state = NULL;
   jobject jPtype = NULL;
   jobject jInitval = NULL;
   jobject jConstraint = NULL;
   jobject jMin;
   jobject jMax;
   jobjectArray jOptions;
   jobject jOpt;
   jobject jParimp;
   int ptrsz;
   int nopt;
   int iopt;

   /* Prepare a byte array containing a pointer to the parameter object. */
   ptrsz = sizeof(cpl_parameter *);
   jState = (*env)->NewByteArray(env, ptrsz);
   if (jState) {
      state = (*env)->GetByteArrayElements(env, jState, NULL);
      memcpy(state, &param, ptrsz);
      (*env)->ReleaseByteArrayElements(env, jState, state, 0);
   }
   else {
      return NULL;
   }

   /* Ascertain the parameter type and its initial value. */
   ptype = cpl_parameter_get_type(param);
   switch (ptype) {
      case CPL_TYPE_BOOL:
         jPtype = ParameterTypeBOOLConstant;
         jInitval = makeBoolean(cpl_parameter_get_default_bool(param));
         break;
      case CPL_TYPE_INT:
         jPtype = ParameterTypeINTConstant;
         jInitval = makeInteger(env, cpl_parameter_get_default_int(param));
         break;
      case CPL_TYPE_DOUBLE:
         jPtype = ParameterTypeDOUBLEConstant;
         jInitval = makeDouble(env, cpl_parameter_get_default_double(param));
         break;
      case CPL_TYPE_STRING:
         jPtype = ParameterTypeSTRINGConstant;
         jInitval = makeString(env, cpl_parameter_get_default_string(param));
         break;
      default:
         (*env)->ThrowNew(env, IllegalStateExceptionClass,
                          "Unknown parameter type" );
         return NULL;
   }

   /* Construct a suitable ParameterConstraint object. */
   pclass = cpl_parameter_get_class(param);
   switch (pclass) {

      /* Value class - no restrictions. */
      case CPL_PARAMETER_CLASS_VALUE:
         jConstraint = NULL;
         break;

      /* Range class - minimum and maximum values are set. */
      case CPL_PARAMETER_CLASS_RANGE:
         jMin = NULL;
         jMax = NULL;
         switch (ptype) {
            case CPL_TYPE_INT:
               jMin = makeInteger(env,
                                  cpl_parameter_get_range_min_int(param));
               jMax = makeInteger(env,
                                  cpl_parameter_get_range_max_int(param));
               break;
            case CPL_TYPE_DOUBLE:
               jMin = makeDouble(env,
                                 cpl_parameter_get_range_min_double(param));
               jMax = makeDouble(env,
                                 cpl_parameter_get_range_max_double(param));
               break;
            default:
               (*env)->ThrowNew(env, IllegalStateExceptionClass,
                                "Bad type for range parameter" );
               return NULL;
         }
         jConstraint = (*env)->NewObject(env, RangeConstraintClass,
                                         RangeConstraintConstructor,
                                         jPtype, jMin, jMax);
         if (!jConstraint) {
            return NULL;
         }
         break;

      /* Enum class - a discrete list of options is set. */
      case CPL_PARAMETER_CLASS_ENUM:
         nopt = cpl_parameter_get_enum_size(param);
         jOptions = (*env)->NewObjectArray(env, nopt, ObjectClass, NULL);
         jOpt = NULL;
         for (iopt = 0; iopt < nopt; iopt++) {
            switch (ptype) {
               case CPL_TYPE_INT:
                  jOpt = makeInteger(env,
                                     cpl_parameter_get_enum_int(param, iopt));
                  break;
               case CPL_TYPE_DOUBLE:
                  jOpt = makeDouble(env,
                                    cpl_parameter_get_enum_double(param, iopt));
                  break;
               case CPL_TYPE_STRING:
                  jOpt = makeString(env,
                                    cpl_parameter_get_enum_string(param, iopt));
                  break;
               default:
                  (*env)->ThrowNew(env, AssertionErrorClass, NULL);
                  return NULL;
            }
            if ((*env)->ExceptionCheck(env)) {
               return NULL;
            }
            (*env)->SetObjectArrayElement(env, jOptions, iopt, jOpt);
         }
         jConstraint = (*env)->NewObject(env, EnumConstraintClass,
                                         EnumConstraintConstructor, jOptions);
         break;

      /* Unknown. */
      default:
         (*env)->ThrowNew(env, IllegalStateExceptionClass,
                          "Unknown parameter class");
         return NULL;
   }

   /* Construct a JNIParameterImp object from the objects we have made. */
   jParimp = (*env)->NewObject(env, JNIParameterImpClass,
                               JNIParameterImpConstructor, jState, jRecipe,
                               jPtype, jConstraint, jInitval);
   if (!jParimp) {
      return NULL;
   }

   /* Initialise the remaining fields in the ParameterImp. */
   (*env)->SetObjectField(env, jParimp, JNIParameterImpContextField,
              (*env)->NewStringUTF(env, cpl_parameter_get_context(param)));
   (*env)->SetObjectField(env, jParimp, JNIParameterImpHelpField,
              (*env)->NewStringUTF(env, cpl_parameter_get_help(param)));
   (*env)->SetObjectField(env, jParimp, JNIParameterImpNameField,
              (*env)->NewStringUTF(env, cpl_parameter_get_name(param)));
   (*env)->SetObjectField(env, jParimp, JNIParameterImpTagField,
              (*env)->NewStringUTF(env, cpl_parameter_get_tag(param)));

   /* Make and return a Parameter object from the ParameterImp. */
   return (*env)->NewObject(env, ParameterClass, ParameterConstructor,
                            jParimp);
}


/*
 * Makes a java Boolean object from a boolean (int) value.
 */

static jobject makeBoolean(int val) {
   return val ? BooleanTRUEConstant : BooleanFALSEConstant;
}


/*
 * Makes a java Integer object from an int value.
 */

static jobject makeInteger(JNIEnv *env, int val) {
   return (*env)->NewObject(env, IntegerClass, IntegerConstructor,
                            (jint) val);
}


/*
 * Makes a java Double object from a double value.
 */

static jobject makeDouble(JNIEnv *env, double val) {
   return (*env)->NewObject(env, DoubleClass, DoubleConstructor,
                            (jdouble) val);
}


/*
 * Makes a java String object from a char* value.
 */

static jobject makeString(JNIEnv *env, const char *val) {
   return (*env)->NewStringUTF(env, val);
}

#undef EXECUTE_CPL_PLUGIN_FUNC
