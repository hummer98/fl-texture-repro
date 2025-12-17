#include <gtest/gtest.h>
#include <gst/gst.h>

// Test GStreamer initialization
TEST(FlTextureReproPluginTest, GStreamerInitialization) {
  // GStreamer should initialize without error
  GError* error = nullptr;
  gboolean result = gst_init_check(nullptr, nullptr, &error);

  EXPECT_TRUE(result);
  EXPECT_EQ(error, nullptr);

  if (error != nullptr) {
    g_error_free(error);
  }
}

// Test GStreamer pipeline parsing
TEST(FlTextureReproPluginTest, PipelineParsing) {
  gst_init(nullptr, nullptr);

  // Test with videotestsrc (doesn't require camera)
  const char* pipeline_str =
      "videotestsrc num-buffers=1 ! "
      "video/x-raw,format=RGBA,width=320,height=240 ! "
      "fakesink";

  GError* error = nullptr;
  GstElement* pipeline = gst_parse_launch(pipeline_str, &error);

  EXPECT_NE(pipeline, nullptr);
  EXPECT_EQ(error, nullptr);

  if (error != nullptr) {
    g_error_free(error);
  }

  if (pipeline != nullptr) {
    gst_object_unref(pipeline);
  }
}

// Test pipeline state changes
TEST(FlTextureReproPluginTest, PipelineStateChanges) {
  gst_init(nullptr, nullptr);

  const char* pipeline_str =
      "videotestsrc num-buffers=5 ! "
      "video/x-raw,format=RGBA,width=320,height=240 ! "
      "fakesink";

  GError* error = nullptr;
  GstElement* pipeline = gst_parse_launch(pipeline_str, &error);

  ASSERT_NE(pipeline, nullptr);

  // Test state transitions
  GstStateChangeReturn ret;

  ret = gst_element_set_state(pipeline, GST_STATE_PAUSED);
  EXPECT_NE(ret, GST_STATE_CHANGE_FAILURE);

  ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  EXPECT_NE(ret, GST_STATE_CHANGE_FAILURE);

  ret = gst_element_set_state(pipeline, GST_STATE_NULL);
  EXPECT_NE(ret, GST_STATE_CHANGE_FAILURE);

  gst_object_unref(pipeline);
}
