# This script can be used as a option to analise how the returns are getting from AM server.
#!/bin/bash
#set -x
clear

### Change with yours values
url_base='https://myAMserver:8443'
url_autenticacao='/openam/json/realms/root/authenticate?realm=myChain2fa'
vcacerts='/etc/ssl/certs/myCA.pem'

if [[ "$1" = "" ]]; then
  echo ""
  echo "You did not provide a username."
  echo ""
  echo -e "What is the username that I should authenticate?"
  read USER
        else
          USER=$1
fi

if [[ "$2" = "" ]]; then
  echo ""
  echo "You did not provide a password."
  echo ""
  echo -e "What is the password for this user?"
  stty -echo
  read PASS
  stty echo
        else
          PASS=$2
fi

if [[ "$3" = "" ]]; then
  echo ""
  echo "You did not provide a OTP."
  echo ""
  echo -e "What is the OTP for this session?"
  read vOTP
        else
          vOTP=$3
fi
echo "================================================================================================"

f_otpOK() {
        otpOK=`echo $1 | grep -c successUrl`
        if [ $otpOK -eq 1 ]; then
           ret_otpOK=1
           echo "---"
           echo "### ret_otpOK=1 !"
           echo "---"
           echo "================================================================================================"
           else
              ret_otpOK=0
              echo "---"
              echo "### ret_otpOK=0 !"
              echo "---"
              echo "================================================================================================"
        fi
}

submit00(){

RET00="$(curl -k -s --cacert $vcacerts --request POST --header 'Content-Type: application/json' --header "Accept-API-Version: resource=2.0, protocol=1.0" --data '{}' ''$url_base$url_autenticacao'')"

}

submit01() {

RET01="$(curl -k -s --cacert $vcacerts --request POST --header "Accept-API-Version: resource=2.0, protocol=1.0" --header 'Content-Type: application/json' ''$url_base$url_autenticacao'' -d '{"authId":"'$1'","callbacks":[{"type": "NameCallback","output":[{"name": "prompt","value": "User Name:"}],"input":[{"name": "IDToken1","value": "'$2'"}]},{"type": "PasswordCallback","output":[{"name": "prompt","value":"Password:"}],"input":[{"name": "IDToken2","value": "'$3'"}]}]}')"

}

submit02(){

RET02="$(curl -k -s --cacert $vcacerts --request POST --header "Accept-API-Version: resource=2.0, protocol=1.0" --header 'Content-Type: application/json' ''$url_base$url_autenticacao'' -d '{"authId":"'$1'","callbacks":[{"type":"NameCallback","output":[{"name":"prompt","value":"Enter verification code:"}],"input":[{"name":"IDToken1","value":"'$2'"}]},{"type":"ConfirmationCallback","output":[{"name":"prompt","value":""},{"name":"messageType","value":0},{"name":"options","value":["Submit"]},{"name":"optionType","value":-1},{"name":"defaultOption","value":0}],"input":[{"name":"IDToken2","value":'$3'}]}]}')"

}

submit00
echo "RET00="$RET00
USER_AM_TOKEN=$(echo "$RET00" | cut -d':' -f 2 | cut -d',' -f 1 | cut -d'"' -f 2)
submit01 $USER_AM_TOKEN $USER $PASS
echo "RET01="$RET01
f_otpOK $RET01
if [ $ret_otpOK -eq 1 ]; then
        echo ""
        echo "-- Second factor is disabled on the AM Server, I will return PAM_SUCCESS "
        echo "-- Access Allowed!"
        echo ""
        exit
        else
                submit02 $USER_AM_TOKEN "" "1"
                echo "RET02="$RET02
                echo "---"
                submit02 $USER_AM_TOKEN $vOTP "0"
                echo "RET02B="$RET02
                f_otpOK $RET02
                if [ $ret_otpOK -eq 1 ]; then
                        echo ""
                        echo "-- Access Allowed!"
                        echo ""
                        else
                                echo ""
                                echo "-- Access Denied!"
                                echo ""
                fi
fi
exit
