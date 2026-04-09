FROM devkitpro/devkitarm:latest

RUN dkp-pacman -S --noconfirm \
      nds-dev \
      3ds-dev \
    && dkp-pacman -Scc --noconfirm

ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=/opt/devkitpro/devkitARM
ENV PATH=${DEVKITPRO}/tools/bin:${DEVKITARM}/bin:${PATH}
