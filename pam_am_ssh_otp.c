#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <syslog.h>
#include <pwd.h>
#include <stdarg.h>
#include <signal.h>

#ifndef PAM_EXTERN
#define PAM_EXTERN
#endif

/** Globals variables **/
char *appname="pam_am_ssh_otp";
char *app_ver="1.0";
CURL *curl;
CURLcode curl_response;
const char *user          = NULL;
const void *usrpwd;
char *tokenvalue ;
static const char *am_url, *cacerts, *otp_size;
char *requestworkstring;
char string01[30], string02[30], string03[3];
struct MemoryStruct chunk;

/************** syslog stuff **********************/
static void log_debug (char * format, ...){
                va_list args;
                va_start (args, format);
                openlog(appname, LOG_PID , LOG_AUTHPRIV );
                vsyslog(LOG_DEBUG, format,args);
                closelog();
                va_end (args);
}

/*** Source https://ben.akrin.com/2FA/2ndfactor.c  ***/
/*** this function is ripped from pam_unix/support.c, it lets us do IO via PAM ***/
int converse( pam_handle_t *pamh, int nargs, struct pam_message **message, struct pam_response **response ) {
    int retval ;
    struct pam_conv *conv ;
    retval = pam_get_item( pamh, PAM_CONV, (const void **) &conv ) ;
    if( retval==PAM_SUCCESS ) {
        retval = conv->conv( nargs, (const struct pam_message **) message, response, conv->appdata_ptr ) ;
    }
    return retval ;
}

/*** Struct memory allocation ***/
struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t grava_retorno(void *contents, size_t size, size_t nmemb, void *userp){
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(ptr == NULL) {
    /* out of memory! */
    log_debug("not enough memory (realloc returned NULL)");
    return 0;
  }
  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}

/*** Custom Request Elements ***/
/*** Source https://curl.haxx.se/docs/manpage.html ***/
void curl_request(const char *vdata){
  chunk.memory = malloc(1);
  chunk.size = 0;
  curl_global_init(CURL_GLOBAL_ALL);
  struct curl_slist *add_http_header=NULL;
  add_http_header = curl_slist_append(add_http_header, "Content-Type: application/json");
  add_http_header = curl_slist_append(add_http_header, "Accept-API-Version: resource=2.0, protocol=1.0");
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, add_http_header);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
  curl_easy_setopt(curl, CURLOPT_URL, am_url);
  curl_easy_setopt(curl, CURLOPT_CAINFO, cacerts);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, vdata);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(vdata));
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, grava_retorno);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
  curl_response = curl_easy_perform(curl);
  if(curl_response != CURLE_OK) {
    log_debug("curl_easy_perform() failed: %s",curl_easy_strerror(curl_response));
  }
}

/*** Replace strings ***/
/*** Source https://github.com/irl/la-cucina/blob/master/str_replace.c***/
char *repl_str(char* string, const char* substr, const char* replacement) {
        char* tok = NULL;
        char* newstr = NULL;
        char* oldstr = NULL;
        int   oldstr_len = 0;
        int   substr_len = 0;
        int   replacement_len = 0;
        newstr = strdup(string);
        substr_len = strlen(substr);
        replacement_len = strlen(replacement);
        if (substr == NULL || replacement == NULL) {
                return newstr;
        }
        while ((tok = strstr(newstr, substr))) {
                oldstr = newstr;
                oldstr_len = strlen(oldstr);
                newstr = (char*)malloc(sizeof(char) * (oldstr_len - substr_len + replacement_len + 1));
                if (newstr == NULL) {
                        free(oldstr);
                        return NULL;
                }
                memcpy(newstr, oldstr, tok - oldstr);
                memcpy(newstr + (tok - oldstr), replacement, replacement_len);
                memcpy(newstr + (tok - oldstr) + replacement_len, tok + substr_len, oldstr_len - substr_len - (tok - oldstr));
                memset(newstr + oldstr_len - substr_len + replacement_len, 0, 1);
                free(oldstr);
        }
        return newstr;
}

/*** Handle first request ***/
char f_handle_data1(){
    requestworkstring = chunk.memory;
    /** Token1**/
    strcpy(string01,"IDToken1\",\"value\":\"\"");
    strcpy(string02,"IDToken1\",\"value\":\"");
    strcpy(string03,"\"");
    strcat(string02,user);
    strcat(string02,string03);
    requestworkstring = repl_str(requestworkstring,string01,string02);
    /** Token2**/
    strcpy(string01,"IDToken2\",\"value\":\"\"");
    strcpy(string02,"IDToken2\",\"value\":\"");
    strcat(string02,usrpwd);
    strcat(string02,string03);
    requestworkstring = repl_str(requestworkstring,string01,string02);
}

/*** Handle second request ***/
char f_handle_data2(){
    requestworkstring = chunk.memory;
    if (strstr(requestworkstring,"IDToken2") == NULL){
        /*** it has Only string IDToken1 - If Authentication -> Settings -> General -> Two Factor Authentication Mandatory = Disable***/
        strcpy(string01,"IDToken1\",\"value\":0");
        strcpy(string02,"IDToken1\",\"value\":1");
        } else {
            /*** it has string IDToken2 - If Authentication -> Settings -> General -> Two Factor Authentication Mandatory = Enable***/
            strcpy(string01,"IDToken2\",\"value\":0");
            strcpy(string02,"IDToken2\",\"value\":1");
    }
    requestworkstring = repl_str(requestworkstring,string01,string02);
}

/*** Handle third request***/
char f_handle_data3(){
    requestworkstring = chunk.memory;
    /** Token1**/
    strcpy(string01,"IDToken1\",\"value\":\"\"");
    strcpy(string02,"IDToken1\",\"value\":\"");
    strcpy(string03,"\"");
    strcat(string02,tokenvalue);
    strcat(string02,string03);
    requestworkstring = repl_str(requestworkstring,string01,string02);
    /** Token2**/
    strcpy(string01,"IDToken2\",\"value\":1");
    strcpy(string02,"IDToken2\",\"value\":0");
    requestworkstring = repl_str(requestworkstring,string01,string02);
}

/*** If token OK server return successUrl ***/
int f_successOK(char *receive_chunkmemory, int ret_successOK) {
    char *successOK = strstr(receive_chunkmemory,"successUrl");
//    log_debug("successOK = %s", successOK);
    if ( successOK != NULL ) {
//		log_debug("  -- Return 1 = PAM_SUCCESS");
        return 1;
            } else {
//                log_debug("  -- Return 0 = PAM_AUTHTOK_ERR");
                return 0;
    }
}

/*** user\device is registred em AM Server ??? ***/
int f_checkIDToken2(char *receive2_chunkmemory, int ret_checkIDToken2) {
    char *checkIDToken2 = strstr(receive2_chunkmemory,"IDToken2");
//    log_debug(" -- checkIDToken2 = %s", checkIDToken2);
    if ( checkIDToken2 == NULL ) {
//                log_debug("  -- Return 26  = PAM_ABORT");
                return 26;
    }
}

/*** Do requests***/
int do_request(){

   int ret_successOK,ret_checkIDToken2 = -1;

//    log_debug(" -- Run request #1 ...");
    curl_request("");
    f_handle_data1();
//    log_debug("f_handle_data1 - chunk memory = %s", chunk.memory);

//    log_debug(" -- Run request #2 ...");
    free(chunk.memory);
    curl_request(requestworkstring);
    f_handle_data2();
//    log_debug("f_handle_data2 - chunk memory = %s", chunk.memory);

    ret_successOK = f_successOK(chunk.memory,ret_successOK);
    if ( ret_successOK == 1) {
       log_debug(" -- Second factor is disabled on the AM Server, I will return PAM_SUCCESS");	
       return 1;
    }
    
    ret_checkIDToken2 = f_checkIDToken2(chunk.memory,ret_checkIDToken2);
    if ( ret_checkIDToken2 == 26 ) {
       log_debug(" -- Check if user\\device is registred em AM Server");
        exit(26);
    }

//    log_debug(" -- Run request #3 ...");
    free(chunk.memory);
    curl_request(requestworkstring);
    f_handle_data3();
//    log_debug("f_handle_data3 - chunk memory = %s", chunk.memory);

//    log_debug(" -- Run last request #4  ...");
    free(chunk.memory);
    curl_request(requestworkstring);
//    log_debug(" -- Last request - chunk memory = %s", chunk.memory);

    ret_successOK = f_successOK(chunk.memory,ret_successOK);
    if ( ret_successOK == 1) {
       return 1;
       } else {
               return 0;
    }
}

/*** Sources http://www.freebsd.no/doc/en/articles/pam/article.html#pam-sample-conv ***/
/***         https://github.com/HarryKodden/pam_otp/blob/master/pam_otp.c           ***/
/***         https://ben.akrin.com/2FA/2ndfactor.c                                  ***/
/***         https://linux.die.net/man/3/pam_get_user                               ***/
/***         https://linux.die.net/man/3/pam_get_item                               ***/
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char *argv[]) {
#ifndef _OPENPAM
    struct pam_conv         *conv;
    const struct pam_message *msgp;
    struct pam_response *resp;
    struct pam_message msg[1],*pmsg[1];
#endif
    struct passwd *pwd        = NULL;
    int pam_err               = PAM_AUTH_ERR;
    char **args, **pam_envlist, **pam_env;
    int retval, *ret_data = NULL;
    int i,k;

    /*** Get arguments from /etc/pam.d/sshd ***/
    for ( i=0; i<argc;i++){
        if (!strncmp("am_url=", argv[i], 7)) {
           am_url = argv[i]+7;
                } else if (!strncmp("cacerts=", argv[i], 8)) {
                                cacerts = argv[i]+8;
                }  else if (!strncmp("otp_size=", argv[i], 9)) {
                        otp_size = argv[i]+9;
        }
    }

    /*** Get User ***/
    if ((pam_err = pam_get_user(pamh, &user, NULL)) != PAM_SUCCESS) {
        return (pam_err);
    }

    /*** Get first password ***/
    retval = pam_get_item(pamh, PAM_AUTHTOK, &usrpwd);
    if (retval != PAM_SUCCESS) {
        log_debug("User not authenticated");
        return retval;
    }

    /*** Get Token #2 -> OTP ***/
    pmsg[0] = &msg[0] ;
    msg[0].msg_style = PAM_PROMPT_ECHO_ON ;
    msg[0].msg = "Enter verification code - OTP : " ;
    resp = NULL ;

    if( (retval = converse(pamh, 1 , pmsg, &resp))!=PAM_SUCCESS ) {
        log_debug("Check parameter ChallengeResponseAuthentication=yes in /etc/ssh/sshd_config");
        return retval ;
    }

    if( resp ) {
       if( (flags & PAM_DISALLOW_NULL_AUTHTOK) && resp[0].resp == NULL ) {
                free( resp );
                return PAM_AUTH_ERR;
        }
        tokenvalue = resp[ 0 ].resp;
        resp[ 0 ].resp = NULL;
        } else {
        return PAM_AUTHTOK_ERR;
    }

    log_debug("---- Debug %s Version %s ----",appname, app_ver);
    log_debug("am_url  = %s",am_url);
    log_debug("cacerts   = %s",cacerts);
    log_debug("otp_size  = %s",otp_size);
    log_debug("User key  = %s", user);
    log_debug("Password  = ********"); /*** If necessary, change to usrpwd, BUT DO NOT USE in production environment ***/
    log_debug("Token OTP = %s", tokenvalue);

    /*** Validate input OTP token size***/
    /*** Mitigate invalid requests to the server ***/
    for ( k =0;  tokenvalue[k] != '\0'; ++k ) ;
       if ( k != atoi(otp_size) )
          return (PAM_AUTHTOK_ERR);

    /*** Starts OTP check ***/
    retval = do_request();
//    log_debug(" -- retval = %d", retval);
    free (tokenvalue);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    curl_global_cleanup();
    if ( retval == 0 ) {
          log_debug("-- Access Denied!");
          return (PAM_AUTHTOK_ERR);
          } else {
             log_debug("-- Access Allowed!");
             return (PAM_SUCCESS);
    }

}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv){
      return PAM_SUCCESS;
}

#ifdef PAM_MODULE_ENTRY
PAM_MODULE_ENTRY(appname);
#endif


