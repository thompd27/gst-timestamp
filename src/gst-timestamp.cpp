/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2020 David Thompson <dave@republicofdave.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

 /**
  * SECTION:timestamp
  *
  * FIXME:Adds a timestamp utf8 text stream to accompany an input image
  *
  * <refsect2>
  * <title>Example launch line</title>
  * |[
  gst-launch-1.0 autovideosrc ! timestamp name=demux demux.textsrc ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=5000000000 ! "text/x-raw, format=utf8" ! filesink location=D:/text.csv demux.videosrc ! queue max-size-bytes=0 max-size-buffers=0 max-size-time=5000000000 ! videoconvert ! x264enc ! qtmux ! filesink location=D:/text.mp4
  * gst-launch-1.0 videotestsrc ! timestamp name=demux demux.textsrc ! text.text_sink demux.videosrc ! textoverlay name=text ! autovideosink 
  * ]|
  * </refsect2>
  */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <string>
#include <chrono>
#include "gst-timestamp.h"

GST_DEBUG_CATEGORY_STATIC(gst_plugin_template_debug);
#define GST_CAT_DEFAULT gst_plugin_template_debug

/* Filter signals and args */
enum
{
	/* FILL ME */
	LAST_SIGNAL
};

enum
{
	PROP_0,
	PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS_ANY
);

static GstStaticPadTemplate src_video_template = GST_STATIC_PAD_TEMPLATE("videosrc",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("video/x-raw")
);

static GstStaticPadTemplate src_subtitle_template = GST_STATIC_PAD_TEMPLATE("textsrc",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS("text/x-raw, format= { utf8 }")
);
#define gst_plugin_template_parent_class parent_class
G_DEFINE_TYPE(GstPluginTemplate, gst_plugin_template, GST_TYPE_ELEMENT);

static void gst_plugin_template_set_property(GObject* object, guint prop_id,
	const GValue* value, GParamSpec* pspec);
static void gst_plugin_template_get_property(GObject* object, guint prop_id,
	GValue* value, GParamSpec* pspec);
static gboolean gst_plugin_template_setcaps(GstPluginTemplate* plugin, GstCaps* caps);

static gboolean gst_plugin_template_sink_event(GstPad* pad, GstObject* parent, GstEvent* event);
static GstFlowReturn gst_plugin_template_chain(GstPad* pad, GstObject* parent, GstBuffer* buf);

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void
gst_plugin_template_class_init(GstPluginTemplateClass* klass)
{
	GObjectClass* gobject_class;
	GstElementClass* gstelement_class;

	gobject_class = (GObjectClass*)klass;
	gstelement_class = (GstElementClass*)klass;

	gobject_class->set_property = gst_plugin_template_set_property;
	gobject_class->get_property = gst_plugin_template_get_property;

	g_object_class_install_property(gobject_class, PROP_SILENT,
		g_param_spec_boolean("silent", "Silent", "Produce verbose output ?",
			FALSE, G_PARAM_READWRITE));

	gst_element_class_set_details_simple(gstelement_class,
		"gst-timestamp",
		"FIXME:Generic",
		"FIXME:Generic Template Element",
		"AUTHOR_NAME AUTHOR_EMAIL");

	gst_element_class_add_pad_template(gstelement_class,
		gst_static_pad_template_get(&src_video_template));
	gst_element_class_add_pad_template(gstelement_class,
		gst_static_pad_template_get(&src_subtitle_template));
	gst_element_class_add_pad_template(gstelement_class,
		gst_static_pad_template_get(&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_plugin_template_init(GstPluginTemplate* filter)
{
	filter->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
	gst_pad_set_event_function(filter->sinkpad,
		GST_DEBUG_FUNCPTR(gst_plugin_template_sink_event));
	gst_pad_set_chain_function(filter->sinkpad,
		GST_DEBUG_FUNCPTR(gst_plugin_template_chain));
	gst_element_add_pad(GST_ELEMENT(filter), filter->sinkpad);

	filter->srcpadVideo = gst_pad_new_from_static_template(&src_video_template, "videosrc");
	gst_element_add_pad(GST_ELEMENT(filter), filter->srcpadVideo);

	filter->srcpadText = gst_pad_new_from_static_template(&src_subtitle_template, "textsrc");
	gst_pad_use_fixed_caps(filter->srcpadText);
	gst_element_add_pad(GST_ELEMENT(filter), filter->srcpadText);

	filter->silent = FALSE;
	filter->frame = 0;
}

static void
gst_plugin_template_set_property(GObject* object, guint prop_id,
	const GValue* value, GParamSpec* pspec)
{
	GstPluginTemplate* filter = GST_PLUGIN_TEMPLATE(object);

	switch (prop_id) {
	case PROP_SILENT:
		filter->silent = g_value_get_boolean(value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

static void
gst_plugin_template_get_property(GObject* object, guint prop_id,
	GValue* value, GParamSpec* pspec)
{
	GstPluginTemplate* filter = GST_PLUGIN_TEMPLATE(object);

	switch (prop_id) {
	case PROP_SILENT:
		g_value_set_boolean(value, filter->silent);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_plugin_template_sink_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
	GstPluginTemplate* filter;
	gboolean ret;

	filter = GST_PLUGIN_TEMPLATE(parent);

	g_print("Received %s event: %" GST_PTR_FORMAT,
		GST_EVENT_TYPE_NAME(event), event);

	switch (GST_EVENT_TYPE(event)) {
	case GST_EVENT_CAPS:
		GstCaps* caps;

		gst_event_parse_caps(event, &caps);
		/* set caps to whatever the sinkpad wants */
		gst_pad_set_caps(filter->srcpadVideo, caps);

		/* and forward */
		ret = gst_pad_event_default(pad, parent, event);
		break;
	case GST_EVENT_EOS:
		/* end-of-stream, we should close down all stream leftovers here */
		//gst_my_filter_stop_processing(filter);
		ret = gst_pad_push_event(filter->srcpadVideo, event);
		ret = gst_pad_push_event(filter->srcpadText, event);
		break;
	default:
		ret = gst_pad_event_default(pad, parent, event);
		break;
	}
	return ret;
}


/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_plugin_template_chain(GstPad* pad, GstObject* parent, GstBuffer* buf)
{
	GstPluginTemplate* filter;
	
	filter = GST_PLUGIN_TEMPLATE(parent);
	
	guint hours, mins, secs, msecs;
	
	hours = (guint)(buf->pts / (GST_SECOND * 60 * 60));
	mins = (guint)((buf->pts / (GST_SECOND * 60)) % 60);
	secs = (guint)((buf->pts / GST_SECOND) % 60);
	msecs = (guint)((buf->pts % GST_SECOND) / (1000 * 1000));
	using namespace std::chrono;
	milliseconds ms = duration_cast<milliseconds>(
		system_clock::now().time_since_epoch()
		);
	//const gchar* timestamp = g_strdup_printf("%u:%02u:%02u.%03u\n", hours, mins, secs, msecs);
	std::string temp = std::to_string(filter->frame) + ',' + std::to_string(ms.count()) + '\n';
	const char* timestamp = temp.c_str();
	GstBuffer* textbuffer = gst_buffer_new_allocate(NULL, strlen(timestamp), NULL);
	//gst_buffer_fill(textbuffer, 1, timestamp, strlen(timestamp));
	GstMapInfo minfo;
	gst_buffer_map(textbuffer, &minfo, GST_MAP_WRITE);
	memcpy(minfo.data, timestamp, strlen(timestamp));
	gst_buffer_unmap(textbuffer, &minfo);

	GST_BUFFER_DURATION(textbuffer) = buf->duration;
	GST_BUFFER_PTS(textbuffer) = buf->pts;
	GST_BUFFER_DTS(textbuffer) = buf->dts;
	//set the current number in the frame
	GST_BUFFER_OFFSET(textbuffer) = buf->offset;
	//g_print("pushing text\n");
	GstFlowReturn textreturn = gst_pad_push(filter->srcpadText, textbuffer); //push the timestamp
	//g_print("pushing video\n");
	GstFlowReturn videoreturn = gst_pad_push(filter->srcpadVideo, buf); //passthrough the video
	//gst_buffer_unref(textbuffer);

	//make sure text pushed ok
	if (textreturn != GST_FLOW_OK) {
		g_print("text flow error: %s\n", textreturn);
		return textreturn;
	}

	//make sure video pushed ok
	if (videoreturn != GST_FLOW_OK) {
		g_print("video flow error\n");
		return videoreturn;
	}
	/* just push out the incoming buffer without touching it */
	filter->frame++;
	return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init(GstPlugin* plugin)
{
	/* debug category for fltering log messages
	 *
	 * exchange the string 'Template plugin' with your description
	 */
	GST_DEBUG_CATEGORY_INIT(gst_plugin_template_debug, "plugin",
		0, "Template plugin");

	return gst_element_register(plugin, "timestamp", GST_RANK_NONE,
		GST_TYPE_PLUGIN_TEMPLATE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "timestamp"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0.0"
#endif
#ifndef GST_LICENSE
#define GST_LICENSE "LGPL"
#endif
#ifndef GST_PACKAGE_NAME
#define GST_PACKAGE_NAME "timestamp"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "myfirstplugin"
#endif

 /* gstreamer looks for this structure to register plugins
  *
  * exchange the string 'Template plugin' with your plugin description
  */
GST_PLUGIN_DEFINE(
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	timestamp,
	"Timestamp plugin",
	plugin_init,
	PACKAGE_VERSION,
	GST_LICENSE,
	GST_PACKAGE_NAME,
	GST_PACKAGE_ORIGIN
)
