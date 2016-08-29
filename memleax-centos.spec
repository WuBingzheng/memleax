# Spec file for Memleax on Red Hat Enterprise Linux.

Name: memleax
Summary: memleax detects memory leak of a running process
Group:  Development/Debuggers
URL: https://github.com/WuBingzheng/memleax
Vendor: Wu Bingzheng
Version: 1.0.1

License: GPL
Release: el7
Source: memleax-%{version}.tar.gz
Requires: libdwarf, elfutils-libelf, libunwind
BuildRequires: libdwarf-devel, elfutils-libelf-devel, libunwind-devel

%description
memleax detects memory leak of a running process.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT/usr/local/
make

%install
rm -rf $RPM_BUILD_ROOT
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/local/bin/memleax
