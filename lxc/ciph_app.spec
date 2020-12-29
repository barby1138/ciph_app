Name: ciph_app
Version: %{_pwver}
Release: 1%{?dist}
Summary: ciph_app LXC container

License: Parallel Wireless Inc
URL: http://www.parallelwireless.com

%description
ciph_app LXC container

%prep
cd %{_topdir}/BUILD
cp -prf %{_topdir}/SOURCES/%{name} .
cd %{name}
/usr/bin/chmod -Rf a+rX,u+w,g-w,o-w .

%install
cd %{name}
install -d  %{buildroot}/tmp/ciph_app/%{version}/
install -v -m 0644 ciph_app/* -D %{buildroot}/tmp/ciph_app/%{version}/ciph_app
install -v -m 0644 libs/* -D %{buildroot}/tmp/ciph_app/%{version}/libs
install -v -m 0744 build-lxc.sh -D %{buildroot}/tmp/ciph_app/%{version}/build-lxc.sh

%files
/tmp/ciph_app/%{version}/

%doc

%post
/tmp/ciph_app/%{version}/build-lxc.sh

%changelog
