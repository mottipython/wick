Name:           wick
Version:        %{version}
Release:        1%{?dist}
Summary:        The Wick, a screensaver from "Anathem"
License:        MIT
Source0:        wick-%{version}.tar.gz
BuildRequires:  gcc, make, pkgconfig(sdl2)
Requires:       swayidle

%description
Calca 3 of Neal Stephenson's novel Anathem: worlds joined by threads along which
knowledge flows one way only. The diagrams of the Calca are built up in turn,
from Simple Protism to the Wick itself, with light running along every thread,
and the Daban Urnud climbs the Wick from Urnud to Arbre.

On Wayland desktops it starts by itself after five minutes idle.
wick-screensaver changes the delay or turns it off.

%prep
%setup -q

%build
make %{?_smp_mflags} CFLAGS="%{optflags}" LDFLAGS="%{?__global_ldflags}"

%install
make install DESTDIR=%{buildroot} PREFIX=%{_prefix} SYSCONFDIR=%{_sysconfdir}
rm -rf %{buildroot}%{_datadir}/doc/wick

%files
%license LICENSE
%doc README.md
%{_bindir}/wick
%{_bindir}/wick-screensaver
%{_datadir}/applications/wick.desktop
%dir %{_datadir}/screensaver-service
%dir %{_datadir}/screensaver-service/savers
%{_datadir}/screensaver-service/savers/wick.desktop
%{_datadir}/icons/hicolor/256x256/apps/wick.png
%config(noreplace) %{_sysconfdir}/xdg/autostart/wick-screensaver.desktop
