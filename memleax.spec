Name:           memleax
Version:        1.1.1
Release:        1%{?dist}
Summary:        Debugs memory leak of a running process

License:        GPLv2
URL:            http://wubingzheng.github.io/memleax/
Source0:        https://github.com/WuBingzheng/memleax/archive/v1.1.1.tar.gz

BuildRequires:  libdwarf-devel, elfutils-libelf-devel, libunwind-devel

ExcludeArch:    ARM-hfp, x86

%description
Memleax debugs memory leak of a running process by attaching it.
It is very convenient to use, and suitable for production environment.
There is no need to recompile the program or restart the target process.


%prep
%setup -q


%build
./configure
make


%install
make install DESTDIR=%{buildroot}


%files
%{_bindir}/memleax
%{_mandir}/man1/memleax.1*


%changelog
* Thu Jan 04 2018 Wu Bingzheng <wubingzheng@gmail.com> - 1.1.1-1.el7.centos
- bugfix: memory leak at unwind cache
* Sun Nov 05 2017 Wu Bingzheng <wubingzheng@gmail.com> - 1.1.0-1.el7.centos
- support x86, armv7, and aarch64 for GNU/Linux, and i386 for FreeBSD
* Sat Feb 28 2017 Wu Bingzheng <wubingzheng@gmail.com> - 1.0.3-1.el7.centos
- add '-lz' in configure for FreeBSD
* Sat Jan 28 2017 Wu Bingzheng <wubingzheng@gmail.com> - 1.0.2-1.el7.centos
- update README.md, and add man page
