Name: ciph_app
Version: 1.0.1
Release: 1%{?dist}
Summary: ciph_app LXC container

License: Parallel Wireless Inc
URL: http://www.parallelwireless.com

%description
ciph_app LXC container

%prep

%install
cd %{name}
install -d  %{buildroot}/tmp/ciph_app/%{version}/
for f in bin/*; do \
install -v -m 0644 "$f" -D %{buildroot}/tmp/ciph_app/%{version}/bin; \
done
for f in lib/*; do \
install -v -m 0644 "$f" -D %{buildroot}/tmp/ciph_app/%{version}/lib; \
done
install -v -m 0744 build_lxc.sh -D %{buildroot}/tmp/ciph_app/%{version}/build_lxc.sh
install -v -m 0744 VERSION -D %{buildroot}/tmp/ciph_app/%{version}/VERSION
install -v -m 0744 config -D %{buildroot}/tmp/ciph_app/%{version}/config

%files
/tmp/ciph_app/%{version}/

%doc

%post
cd /tmp/ciph_app/%{version}
./build_lxc.sh

%changelog
