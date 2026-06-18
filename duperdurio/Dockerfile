FROM devkitpro/devkitarm:latest

RUN dkp-pacman -S --noconfirm \
      3ds-dev \
      tex3ds \
      general-tools \
    && dkp-pacman -Scc --noconfirm

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=${DEVKITPRO}/tools/bin:${DEVKITARM}/bin:${PATH}
