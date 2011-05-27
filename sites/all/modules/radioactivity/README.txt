$Id: README.txt,v 1.2 2008/04/05 18:15:53 skiminki Exp $

OVERVIEW
========

This module is written to provide better metrics for node ranking
based on viewing rate. The rank information may be used on views to
provide different 'most read' lists, etc.

Particularly on news sites, the node_counter.daycount isn't really
adequate. On those sites, metrics that measure hour-scale and
week-scale activity might become important additions. Furthermore, the
fact that node_counter.daycount gets zeroed once a day makes it
questionable source for 'most read' lists. Resetting the daycount
makes those lists erratic until user activity has normalized the
situation.


SUPPORTED DATABASE BACKENDS
===========================

This module is developed for:
- MySQL 5.0.XX
- PostgreSQL 8.0.XX
Older versions might also work.


ALGORITHM
=========

The node ranking is based on radioactivity model. The algorithm
behind the model is pretty simple:

* Viewing a node adds energy to it and makes it 'hotter'.

* The energy decays at a rate defined by the half-life period, making
  it 'cooler'. For example, a click is worth 1 energy unit at the time
  of clicking, 0.5 energy units after one half-life period, 0.25
  energy units after two half-life periods, and so on.

Therefore, nodes that get lots of clicks stay hotter while inactive
nodes stay cooler. Note that the model is continuous, meaning that the
click energy is not degraded only once per half-life. Actually, it may
be degraded as often as required to get better precision. Degrading
more often means less energy is reduced per iteration.


IN PRACTICE
===========

By keeping half-life period short, let's say one hour, you'll get a
metric that reflects current node view rate. Using that metric in a
view, you can create a "most viewed nodes right now" list. By keeping
longer half-life, let's say 12 hours, you can setup a view that
roughly reflects todays most read nodes. The module supports setting
up multiple different decay rates.

There's a decay granularity setting in the
'admin/settings/radioactivity' page. You might want to keep that
somewhere like 10% of the minimum half-life time of your profiles to
get reasonable precision. You might go as low as 60 seconds on a busy
site, but keep an eye on the cron run duration. This granularity value
is the minimum interval between decay calculations. The actual
interval is, however, the minimum of cron-run period and this value.


ACKNOWLEDGEMENTS
================

The development of this module was funded by Kustannusosakeyhtiö Uusi
Suomi, which runs news site www.uusisuomi.fi (in Finnish).
