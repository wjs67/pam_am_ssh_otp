# pam_am_ssh_otp
Provide PAM module, configuring 2nd factor authentication on SSH with ForgeRock® Authenticator and ForgeRock® Access Manager.

If you find any strange behaviour, please give me feedback so that I can fix them. Also welcome suggestions and criticism.

# Premises:
- Active LDAP and ForgeRock® Access Manager servers, as well as the communication between them;
- ForgeRock® Authenticator installed on your mobile phone;
- Authentication chain for OTP created on the AM server - [like this guide](https://backstage.forgerock.com/docs/am/6.5/authentication-guide/#authn-mfa-chain-oath); 
- Objectclass and attributes from [oath2fa.schema](https://github.com/wjs67/pam_am_ssh_otp/blob/master/oath2fa.schema) in your LDAP server;

# To compile:
 ## build requires:
 ~~~
  * gcc 
  * make 
  * glibc-devel 
  * pam-devel 
  * libcurl-devel 
~~~

## Command:
~~~
gcc -shared -o pam_am_ssh_otp.so -fPIC pam_am_ssh_otp.c -lpam -lcurl
ld --shared -x -lc -o /lib64/security/pam_am_ssh_otp.so pam_am_ssh_otp.o -lpam -lcurl
~~~

- More details, see [pam_am_ssh_otp.spec](https://github.com/wjs67/pam_am_ssh_otp/blob/master/pam_am_ssh_otp.spec) file.

# RPM and source package 
- Are available at https://build.opensuse.org/package/show/home:wellingtonsilva67/pam_am_ssh_otp , for the following distributions:
  * Fedora 31 and 32;
  * SLE_11_SP4;
  * SLE_12_SP4 and SP5;
  * SLE_15, SP1 and SP2;
  * OpenSUSE_Leap_15.2 .

# Manual steps

## Edit the file /etc/pam.d/sshd like this sample:
~~~
#%PAM-1.0
auth        required    pam_debug.so
auth        requisite   pam_nologin.so
#auth       include     common-auth
auth        required    pam_env.so debug
auth        optional    pam_group.so
auth        sufficient  pam_unix.so
auth        requisite   pam_ldap.so use_first_pass
auth 	required 	pam_am_ssh_otp.so "am_url+path_rest_api+myrealm2fa" cacerts="path_cacerts" otp_size="var_otp_size"
~~~

Module pam_am_ssh_otp parameters:
Parameter | Meaning | Example
--- | --- | ---
am_url | AM host + Port SSL | am_url=https://myserverAM:8443
path_rest_api | Fixed value for REST endpoint | /openam/json/realms/root/authenticate?realm=
myrealm2fa  | subrealm of the top-level with 2fa chain in AM server | my2fa
path_cacerts  | CA-signed certificate | /etc/ssl/certs/chain_myAC.pem
var_otp_size |  AM Server > Realm Name > Authentication > Modules > Module 2fa - ForgeRock Authenticator (OATH) >  One Time Password Length = "this value" | 6 (default)

The line would then look like:
~~~
auth required pam_am_ssh_otp.so am_url=https://myserverAM:8443/openam/json/realms/root/authenticate?realm=my2fa cacerts=/etc/ssl/certs/chain_myAC.pem otp_size=6
~~~

## Check this parameters in /etc/ssh/sshd_config :
~~~
AuthenticationMethods keyboard-interactive:pam
ChallengeResponseAuthentication yes
UsePAM yes
~~~



Main reference sources:
~~~
http://pubs.opengroup.org/onlinepubs/8329799/chap5.htm   - PAM Status Code
http://www.freebsd.no/doc/en/articles/pam/article.html
http://www.freebsd.no/doc/en/articles/pam/article.html#pam-sample-conv
http://www.rkeene.org/projects/info/wiki/222
https://backstage.forgerock.com/docs/am/5/AM-5-Oauth2-Guide.pdf
https://backstage.forgerock.com/docs/am/6.5/authentication-guide/#about-authentication-modules-and-chains
https://backstage.forgerock.com/docs/am/6.5/authentication-guide/#configure-authn-chains
https://backstage.forgerock.com/docs/am/6.5/deployment-planning-guide/
https://backstage.forgerock.com/docs/am/6.5/dev-guide/#chap-dev-introduction   - Introducing REST
https://ben.akrin.com/2FA/2ndfactor.c
https://docs.oracle.com/cd/E19253-01/816-4863/emrbk/index.html   - PAM Writing Conversation Functions
https://forum.forgerock.com/2016/07/little-things-authentication-chains/
https://github.com/CERN-CERT/pam_2fa/blob/master/module_conf.c
https://github.com/HarryKodden/pam_otp
https://github.com/duykhoa95/LIB-PAM-C
https://gitlab.gnugen.ch/fvessaz/pkg-pam/blob/bec07bdd0b392a014bf60ac32bff82e33b5ccf3f/modules/pam_unix/support.c
https://mirrors.edge.kernel.org/pub/linux/libs/pam/pre/modules/
https://searchcode.com/file/96927934/modules/pam_exec/pam_exec.c
https://stackoverflow.com/questions/2329571/c-libcurl-get-output-into-a-string
https://wikis.forgerock.org/confluence/display/openam/Invoking+HOTP+over+REST
~~~
