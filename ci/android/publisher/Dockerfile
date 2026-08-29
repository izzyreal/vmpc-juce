FROM ruby:3.3-slim-bookworm@sha256:df209d82bb55aeb26b9b08922ee22c15f19b3458455690132dc07a5c52dc1c99 AS builder

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/vmpc-fastlane

COPY ci/android/publisher/Gemfile ci/android/publisher/Gemfile.lock ./

RUN bundle config set deployment true \
    && bundle config set path /usr/local/bundle \
    && bundle config set without development \
    && bundle install

FROM ruby:3.3-slim-bookworm@sha256:df209d82bb55aeb26b9b08922ee22c15f19b3458455690132dc07a5c52dc1c99

ENV BUNDLE_GEMFILE=/opt/vmpc-fastlane/Gemfile \
    FASTLANE_OPT_OUT_USAGE=1 \
    FASTLANE_SKIP_UPDATE_CHECK=1 \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

WORKDIR /opt/vmpc-fastlane

COPY --from=builder /usr/local/bundle /usr/local/bundle
COPY ci/android/publisher/Gemfile ci/android/publisher/Gemfile.lock ./

RUN bundle exec fastlane --version
