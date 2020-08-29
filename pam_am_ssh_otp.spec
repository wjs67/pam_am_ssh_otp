Summary:	2FA for SSH authentication with ForgeRock® Authenticator
Name:	pam_am_ssh_otp
License:	MIT
Version:	1.0
Release:	0.%{_release}
Group:	Applications/System
AutoReqProv:	no
BuildRequires:	pam-devel
BuildRequires:	gcc make glibc-devel pkgconfig
BuildRequires:  gcc
BuildRequires:  pam-devel
BuildRequires:  libcurl-devel
BuildRequires:  openssh
BuildRequires:  rsyslog
BuildRequires:	util-linux-systemd
BuildRoot:	%{_tmppath}/%{name}-%{version}-build
Requires:	pam  curl
Source:	%{name}-%{version}.tar.gz


%description
Provide PAM module, configuring 2nd factor authentication on SSH connections with ForgeRock® Authenticator and ForgeRock® Access Manager

%prep
%setup -q -n "%{name}-%{version}"

%build 
gcc -shared -o %{name}.so -fPIC %{name}.c -lpam -lcurl

%install
mkdir -p $RPM_BUILD_ROOT/%{_lib}/security
install -m 755 %{name}*.so $RPM_BUILD_ROOT/%{_lib}/security

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/%{_lib}/security/%{name}.so

%pre

%post

%preun

%postun

%changelog
* Fri Aug 28 2020 Wellington J.Silva <wellingtonsilva@bb.com.br> - 1.0-0
- packaged pam_am_ssh_otp version 1.0 using the buildservice spec file wizard
- provide PAM module, configuring 2nd factor authentication on SSH with ForgeRock® Authenticator and ForgeRock® Access Manager


