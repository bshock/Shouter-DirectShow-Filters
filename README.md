# Shouter-DirectShow-Filters
DirectShow filters for using the Shout protocol to live-stream audio or video to streaming servers such as Icecast.

Icecast (http://icecast.org/) is a quick and easy way to set up your own private audio or video channel online, using the Windows platform. There are several Windows applications such as EZStream (http://icecast.org/ezstream/) that will allow you to stream pre-recorded audio or video files via the Shout protocol to Icecast. However, there are fewer ways in Windows to Shout-stream live content (such as webcam video) to Icecast.

I've written two very simple DirectShow filters to accomplish this: MP3ShoutStream.dll, which handles MP3 audio, and 
	OggShoutStream.dll, which handles Ogg-Theora video (as well as Ogg-Vorbis audio).
	
I'm going to assume you know how to link up DirectShow filters into a graph to use this, either programmatically or with the GraphEdit test application from Microsoft. If you create a DirectShow graph with an audio or video source, connect it to the correct DirectShow encoder, and then link that to OggShoutStream.dll, you should be able to send live audio or video to Icecast, and from broadcast it over the Internet so that simple HTML5 browser pages can play it. (More detailed instructions are in the files themselves.)
