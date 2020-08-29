# pam_am_ssh_otp
Provide PAM module, configuring 2nd factor authentication on SSH with ForgeRock® Authenticator and ForgeRock® Access Manager

The pam_am_ssh_otp.so module is installed in /lib64/security

The file /etc/pam.d/sshd would then look like:
~~~
#%PAM-1.0
auth        required    pam_debug.so
auth        requisite   pam_nologin.so
#auth       include     common-auth
auth        required    pam_env.so debug
auth        optional    pam_group.so
auth        sufficient  pam_unix.so
auth        requisite   pam_ldap.so use_first_pass
## auth 	required 	pam_am_ssh_otp.so "am_url+path_rest_api+myrealm2fa" cacerts="path_cacerts" otp_size="var_otp_size"
auth        required    pam_am_ssh_otp.so am_url=https://myserverAM:8443/openam/json/realms/root/authenticate?realm=my2fa cacerts=/etc/ssl/certs/chain_myAC.pem otp_size=6
~~~

Check this parameters in /etc/ssh/sshd_config
~~~
AuthenticationMethods keyboard-interactive:pam
ChallengeResponseAuthentication yes
UsePAM yes
~~~

Configuration settings:
Config | Meaning | Example
--- | --- | ---
am_url | AM host + Port SSL | am_url=https://myserverAM:8443
path_rest_api | Fixed value for REST endpoint | /openam/json/realms/root/authenticate?realm=
myrealm2fa  | subrealm of the top-level with 2fa chain in AM server | my2fa
path_cacerts  | CA-signed certificate | /etc/ssl/certs/chain_myAC.pem
var_otp_size |  AM Server > Realm Name > Authentication > Modules > Module 2fa - ForgeRock Authenticator (OATH) >  One Time Password Length = "this value" | 6 (default)

To compile, see "pam_am_ssh_otp.spec" file

RPM files available at https://build.opensuse.org/package/show/home:wellingtonsilva67/pam_am_ssh_otp
* Fedora 31 and 32;
* SLE_11_SP4;
* SLE_12_SP4 and SP5;
* SLE_15, SP1 and SP2;
* OpenSUSE_Leap_15.2 .
