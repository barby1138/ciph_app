Name: ciph_app
Version: %{_ver}
Release: %{_rel}
Summary: ciph_app LXC container

License: Parallel Wireless Inc
URL: http://www.parallelwireless.com

%description
ciph_app LXC container

%prep

%install
cd %{name}
install -d  %{buildroot}/tmp/ciph_app/%{version}/
install -d  %{buildroot}/tmp/ciph_app/%{version}/bin/
install -d  %{buildroot}/tmp/ciph_app/%{version}/lib/
install -d  %{buildroot}/tmp/ciph_app/%{version}/svc/
install -d  %{buildroot}/tmp/ciph_app/%{version}/log/
for f in bin/*; do \
install -v -m 0644 "$f" -D %{buildroot}/tmp/ciph_app/%{version}/bin; \
done
for f in lib/*; do \
install -v -m 0644 "$f" -D %{buildroot}/tmp/ciph_app/%{version}/lib; \
done
install -v -m 0744 install_lxc.sh -D %{buildroot}/tmp/ciph_app/%{version}/install_lxc.sh
install -v -m 0644 VERSION -D %{buildroot}/tmp/ciph_app/%{version}/VERSION
install -v -m 0644 config -D %{buildroot}/tmp/ciph_app/%{version}/config

install -v -m 0644 lxc/rootfs_centos-7-amd64.tar.gz -D %{buildroot}/tmp/ciph_app/%{version}/lxc/rootfs_centos-7-amd64.tar.gz

install -v -m 0644 svc/ciph_app.service -D %{buildroot}/tmp/ciph_app/%{version}/svc/ciph_app.service
install -v -m 0644 svc/ciph_app.sh -D %{buildroot}/tmp/ciph_app/%{version}/svc/ciph_app.sh

install -v -m 0644 log/syslog -D %{buildroot}/tmp/ciph_app/%{version}/log/syslog

%files
/tmp/ciph_app/%{version}/

%doc

%post
cd /tmp/ciph_app/%{version}
./install_lxc.sh

%changelog
