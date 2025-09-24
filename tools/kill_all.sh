#!/usr/bin/env bash
pkill -f server_sim || true
pkill -f client_sim || true
pkill -f "/lb" || true
