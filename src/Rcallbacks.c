#include <stdio.h>

#include "jri.h"
#include "globals.h"

#include "org_rosuda_JRI_Rengine.h"
#include <R_ext/Parse.h>

#ifndef Win32
#include <R_ext/eventloop.h>
#endif


#ifdef Win32
#include <Startup.h>
#else
/* from Defn.h */
extern Rboolean R_Interactive;   /* TRUE during interactive use*/

extern FILE*    R_Consolefile;   /* Console output file */
extern FILE*    R_Outputfile;   /* Output file */
extern char*    R_TempDir;   /* Name of per-session dir */

/* from src/unix/devUI.h */

extern void (*ptr_R_Suicide)(char *);
extern void (*ptr_R_ShowMessage)();
extern int  (*ptr_R_ReadConsole)(char *, unsigned char *, int, int);
extern void (*ptr_R_WriteConsole)(char *, int);
extern void (*ptr_R_ResetConsole)();
extern void (*ptr_R_FlushConsole)();
extern void (*ptr_R_ClearerrConsole)();
extern void (*ptr_R_Busy)(int);
/* extern void (*ptr_R_CleanUp)(SA_TYPE, int, int); */
extern int  (*ptr_R_ShowFiles)(int, char **, char **, char *, Rboolean, char *);
extern int  (*ptr_R_ChooseFile)(int, char *, int);
extern void (*ptr_R_loadhistory)(SEXP, SEXP, SEXP, SEXP);
extern void (*ptr_R_savehistory)(SEXP, SEXP, SEXP, SEXP);
#endif

#ifndef checkArity
#define checkArity               Rf_checkArity
#endif
#ifndef errorcall
#define errorcall                Rf_errorcall
#endif

/* this method is used rather for debugging purposes - it finds the correct JNIEnv for the current thread. we still have some threading issues to solve, becuase eenv!=env should never happen (uncontrolled), because concurrency issues arise */
static JavaVM *jvm=0;

JNIEnv *checkEnvironment()
{
    JNIEnv *env;
    jsize l;
    jint res;
    
    if (!jvm) { // we're hoping that the JVM pointer won't change :P we fetch it just once
        res= JNI_GetCreatedJavaVMs(&jvm, 1, &l);
        if (res!=0) {
            fprintf(stderr, "JNI_GetCreatedJavaVMs failed! (%d)\n",res); return;
        }
        if (l<1) {
            fprintf(stderr, "JNI_GetCreatedJavaVMs said there's no JVM running!\n"); return;
        }
    }
    res = (*jvm)->AttachCurrentThread(jvm, (void*) &env, 0);
    if (res!=0) {
        fprintf(stderr, "AttachCurrentThread failed! (%d)\n",res); return;
    }
    if (eenv!=env)
        fprintf(stderr, "Warning! eenv=%x, but env=%x - different environments encountered!\n", eenv, env);
    return env;
}

int Re_ReadConsole(char *prompt, unsigned char *buf, int len, int addtohistory)
{
	jstring r,s;
	jmethodID mid;
    JNIEnv *lenv=checkEnvironment();
	
    if (!lenv || !engineObj) return -1;
	
	jri_checkExceptions(lenv, 1);
	mid=(*lenv)->GetMethodID(eenv, engineClass, "jriReadConsole", "(Ljava/lang/String;I)Ljava/lang/String;");
#ifdef JRI_DEBUG
	printf("jriReadconsole mid=%x\n", mid);
#endif
	jri_checkExceptions(lenv, 0);
	if (!mid) return -1;
		
	s=(*lenv)->NewStringUTF(eenv, prompt);
	r=(jstring) (*lenv)->CallObjectMethod(lenv, engineObj, mid, s, addtohistory);
	jri_checkExceptions(lenv, 1);
	(*lenv)->DeleteLocalRef(lenv, s);
	jri_checkExceptions(lenv, 0);
	if (r) {
		const char *c=(*lenv)->GetStringUTFChars(lenv, r, 0);
		if (!c) return -1;
		{
			int l=strlen(c);
			strncpy(buf, c, (l>len-1)?len-1:l);
			buf[(l>len-1)?len-1:l]=0;
#ifdef JRI_DEBUG
			printf("Re_ReadConsole succeeded: \"%s\"\n",buf);
#endif
		}
		(*lenv)->ReleaseStringUTFChars(lenv, r, c);
		(*lenv)->DeleteLocalRef(lenv, r);
		return 1;
    }
    return -1;
}

void Re_Busy(int which)
{
	jmethodID mid;
    JNIEnv *lenv=checkEnvironment();
    jri_checkExceptions(lenv, 1);
    mid=(*lenv)->GetMethodID(lenv, engineClass, "jriBusy", "(I)V");
    jri_checkExceptions(lenv, 0);
#ifdef JRI_DEBUG
	printf("jriBusy mid=%x\n", mid);
#endif
	if (!mid) return;
	(*lenv)->CallVoidMethod(lenv, engineObj, mid, which);
    jri_checkExceptions(lenv, 1);
}

void Re_WriteConsole(char *buf, int len)
{
    JNIEnv *lenv=checkEnvironment();
    jri_checkExceptions(lenv, 1);
    jstring s=(*lenv)->NewStringUTF(lenv, buf);
    jmethodID mid=(*lenv)->GetMethodID(lenv, engineClass, "jriWriteConsole", "(Ljava/lang/String;)V");
    jri_checkExceptions(lenv, 0);
#ifdef JRI_DEBUG
	printf("jriWriteConsole mid=%x\n", mid);
#endif
    if (!mid) return;
	(*lenv)->CallVoidMethod(lenv, engineObj, mid, s);
    jri_checkExceptions(lenv, 1);
    (*lenv)->DeleteLocalRef(lenv, s);
}

/* Indicate that input is coming from the console */
void Re_ResetConsole()
{
}

/* Stdio support to ensure the console file buffer is flushed */
void Re_FlushConsole()
{
    JNIEnv *lenv=checkEnvironment();
    jri_checkExceptions(lenv, 1);
    jmethodID mid=(*lenv)->GetMethodID(lenv, engineClass, "jriFlushConsole", "()V");
    jri_checkExceptions(lenv, 0);
#ifdef JRI_DEBUG
	printf("jriWriteconsole mid=%x\n", mid);
#endif
	if (!mid) return;
	(*lenv)->CallVoidMethod(lenv, engineObj, mid);
    jri_checkExceptions(lenv, 1);
}

/* Reset stdin if the user types EOF on the console. */
void Re_ClearerrConsole()
{
}

int Re_ChooseFile(int new, char *buf, int len)
{
    JNIEnv *lenv=checkEnvironment();
	
    if (lenv && engineObj) {
		jmethodID mid;
		jri_checkExceptions(lenv, 1);
		mid=(*lenv)->GetMethodID(eenv, engineClass, "jriChooseFile", "(I)Ljava/lang/String;");
#ifdef JRI_DEBUG
		printf("jriChooseFile mid=%x\n", mid);
#endif
		jri_checkExceptions(lenv, 0);
		if (mid) {
			jstring r=(jstring) (*lenv)->CallObjectMethod(lenv, engineObj, mid, new);
			jri_checkExceptions(lenv, 1);
			if (r) {
				int slen=-1;
				const char *c=(*lenv)->GetStringUTFChars(lenv, r, 0);
				if (c) {
					slen=strlen(c);
					strncpy(buf, c, (slen>len-1)?len-1:slen);
					buf[(slen>len-1)?len-1:slen]=0;
#ifdef JRI_DEBUG
					printf("Re_ChooseFile succeeded: \"%s\"\n",buf);
#endif
				}
				(*lenv)->ReleaseStringUTFChars(lenv, r, c);
				(*lenv)->DeleteLocalRef(lenv, r);
				jri_checkExceptions(lenv, 0);
				return slen;
			}
		}
    }
	
	/* "native" fallback if there's no such method */
	{
		int namelen;
		char *bufp;
		R_ReadConsole("Enter file name: ", (unsigned char *)buf, len, 0);
		namelen = strlen(buf);
		bufp = &buf[namelen - 1];
		while (bufp >= buf && isspace((int)*bufp))
			*bufp-- = '\0';
		return strlen(buf);
	}
}

void Re_ShowMessage(char *buf)
{
	jstring s;
	jmethodID mid;
    JNIEnv *lenv=checkEnvironment();
	
    jri_checkExceptions(lenv, 1);
    s=(*lenv)->NewStringUTF(lenv, buf);
    mid=(*lenv)->GetMethodID(lenv, engineClass, "jriShowMessage", "(Ljava/lang/String;)V");
    jri_checkExceptions(lenv, 0);
#ifdef JGR_DEBUG
    printf("jriShowMessage mid=%x\n", mid);
#endif
    if (mid)
        (*lenv)->CallVoidMethod(eenv, engineObj, mid, s);
    jri_checkExceptions(lenv, 0);
    if (s) (*lenv)->DeleteLocalRef(eenv, s);
}

void Re_read_history(char *buf)
{
}

void Re_loadhistory(SEXP call, SEXP op, SEXP args, SEXP env)
{

    SEXP sfile;
    char file[PATH_MAX], *p;

    /*
    checkArity(op, args);
    sfile = CAR(args);
    if (!isString(sfile) || LENGTH(sfile) < 1)
        errorcall(call, "invalid file argument");
    p = R_ExpandFileName(CHAR(STRING_ELT(sfile, 0)));
    if(strlen(p) > PATH_MAX - 1)
        errorcall(call, "file argument is too long");
    strcpy(file, p);
     */
}

void Re_savehistory(SEXP call, SEXP op, SEXP args, SEXP env)
{
	jmethodID mid;
	jstring s;
    JNIEnv *lenv=checkEnvironment();
	
    jri_checkExceptions(lenv, 1);
    mid=(*lenv)->GetMethodID(lenv, engineClass, "jriSaveHistory", "(Ljava/lang/String;)V");
    jri_checkExceptions(lenv, 0);
#ifdef JRI_DEBUG
	printf("jriSaveHistory mid=%x\n", mid);
#endif
	if (!mid)
		errorcall(call, "can't find jriSaveHistory method");		

	{
		SEXP sfile;
		char file[PATH_MAX], *p;
		
		checkArity(op, args);
		sfile = CAR(args);
		if (!isString(sfile) || LENGTH(sfile) < 1)
			errorcall(call, "invalid file argument");
		p = R_ExpandFileName(CHAR(STRING_ELT(sfile, 0)));
		if(strlen(p) > PATH_MAX - 1)
			errorcall(call, "file argument is too long");	
		s=(*lenv)->NewStringUTF(lenv, p);
	}
	(*lenv)->CallVoidMethod(lenv, engineObj, mid, s);
    jri_checkExceptions(lenv, 1);
	if (s)
		(*lenv)->DeleteLocalRef(lenv, s);
	
	/*
    strcpy(file, p);
        write_history(file);
        history_truncate_file(file, R_HistorySize);
     */
}


/*
 R_CleanUp is invoked at the end of the session to give the user the
 option of saving their data.
 If ask == SA_SAVEASK the user should be asked if possible (and this
                                                            option should not occur in non-interactive use).
 If ask = SA_SAVE or SA_NOSAVE the decision is known.
 If ask = SA_DEFAULT use the SaveAction set at startup.
 In all these cases run .Last() unless quitting is cancelled.
 If ask = SA_SUICIDE, no save, no .Last, possibly other things.
 */

/*
void Re_CleanUp(SA_TYPE saveact, int status, int runLast)
{
    unsigned char buf[1024];
    char * tmpdir;

    if(saveact == SA_DEFAULT)
        saveact = SaveAction;

    if(saveact == SA_SAVEASK) {
        if(R_Interactive) {
qask:
            R_ClearerrConsole();
            R_FlushConsole();
            R_ReadConsole("Save workspace image? [y/n/c]: ",
                          buf, 128, 0);
            switch (buf[0]) {
                case 'y':
                case 'Y':
                    saveact = SA_SAVE;
                    break;
                case 'n':
                case 'N':
                    saveact = SA_NOSAVE;
                    break;
                case 'c':
                case 'C':
                    jump_to_toplevel();
                    break;
                default:
                    goto qask;
            }
        } else
            saveact = SaveAction;
    }
    switch (saveact) {
        case SA_SAVE:
            if(runLast) R_dot_Last();
            if(R_DirtyImage) R_SaveGlobalEnv();
                 stifle_history(R_HistorySize);
                 write_history(R_HistoryFile);
                 break;
        case SA_NOSAVE:
            if(runLast) R_dot_Last();
            break;
        case SA_SUICIDE:
        default:
            break;
    }
    R_RunExitFinalizers();
    CleanEd();
    if(saveact != SA_SUICIDE) KillAllDevices();
    if((tmpdir = getenv("R_SESSION_TMPDIR"))) {
        snprintf((char *)buf, 1024, "rm -rf %s", tmpdir);
        R_system((char *)buf);
    }
    if(saveact != SA_SUICIDE && R_CollectWarnings)
        PrintWarnings();
    fpu_setup(FALSE);

    exit(status);
}

void Rstd_Suicide(char *s)
{
    REprintf("Fatal error: %s\n", s);
    R_CleanUp(SA_SUICIDE, 2, 0);
}

*/
int Re_ShowFiles(int nfile, 		/* number of files */
                 char **file,		/* array of filenames */
                 char **headers,	/* the `headers' args of file.show. Printed before each file. */
                 char *wtitle,          /* title for window = `title' arg of file.show */
                 Rboolean del,	        /* should files be deleted after use? */
                 char *pager)		/* pager to be used */
{
    return 1;
}
