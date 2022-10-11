#
# spec file for package mingw64-udjat-sqlite
#
# Copyright (c) 2015 SUSE LINUX GmbH, Nuernberg, Germany.
# Copyright (C) <2008> <Banco do Brasil S.A.>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via sqliteession://bugs.opensuse.org/
#

Summary:		User/Session module for udjat 
Name:			mingw64-udjat-sqlite
Version:		1.0
Release:		0
License:		LGPL-3.0
Source:			udjat-module-sqlite-%{version}.tar.xz

URL:			https://github.com/PerryWerneck/udjat-module-sqlite

Group:			Development/Libraries/C and C++
BuildRoot:		/var/tmp/%{name}-%{version}

BuildRequires:	autoconf >= 2.61
BuildRequires:	automake
BuildRequires:	libtool
BuildRequires:	binutils
BuildRequires:	coreutils
BuildRequires:	gcc-c++

BuildRequires:	mingw64-cross-binutils
BuildRequires:	mingw64-cross-gcc
BuildRequires:	mingw64-cross-gcc-c++
BuildRequires:	mingw64-cross-pkg-config
BuildRequires:	mingw64-filesystem
BuildRequires:	mingw64-gettext-tools
BuildRequires:	mingw64-libudjat-devel
BuildRequires:	mingw64-sqlite-devel

%description
SQLite module for udjat

#---[ Build & Install ]-----------------------------------------------------------------------------------------------

%prep
%setup -n udjat-module-sqlite-%{version}

NOCONFIGURE=1 \
	./autogen.sh

%{_mingw64_configure}

%build
make all %{?_smp_mflags}

%{_mingw64_strip} \
	--strip-all \
	.bin/Release/*.dll

%install
make DESTDIR=%{buildroot} install

%files
%defattr(-,root,root)
%{_mingw64_libdir}/udjat-modules/*.dll

%changelog

