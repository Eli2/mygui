/*!
	@file
	@author		George Evmenov
	@date		07/2009
*/

#ifndef MYGUI_OPENGL3_RENDER_MANAGER_H_
#define MYGUI_OPENGL3_RENDER_MANAGER_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_RenderFormat.h"
#include "MyGUI_IVertexBuffer.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_OpenGL3ImageLoader.h"

namespace MyGUI
{

	class OpenGL3RenderManager :
		public RenderManager,
		public IRenderTarget
	{
	public:
		OpenGL3RenderManager();

		void initialise(OpenGL3ImageLoader* _loader = nullptr);
		void shutdown();

		static OpenGL3RenderManager& getInstance();
		static OpenGL3RenderManager* getInstancePtr();

		/** @see RenderManager::getViewSize */
		virtual const IntSize& getViewSize() const;

		/** @see RenderManager::getVertexFormat */
		virtual VertexColourType getVertexFormat();

		/** @see RenderManager::isFormatSupported */
		virtual bool isFormatSupported(PixelFormat _format, TextureUsage _usage);

		/** @see RenderManager::createVertexBuffer */
		virtual IVertexBuffer* createVertexBuffer();
		/** @see RenderManager::destroyVertexBuffer */
		virtual void destroyVertexBuffer(IVertexBuffer* _buffer);

		/** @see RenderManager::createTexture */
		virtual ITexture* createTexture(const std::string& _name);
		/** @see RenderManager::destroyTexture */
		virtual void destroyTexture(ITexture* _texture);
		/** @see RenderManager::getTexture */
		virtual ITexture* getTexture(const std::string& _name);


		/** @see IRenderTarget::begin */
		virtual void begin();
		/** @see IRenderTarget::end */
		virtual void end();
		/** @see IRenderTarget::doRender */
		virtual void doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count);
		/** @see IRenderTarget::getInfo */
		virtual const RenderTargetInfo& getInfo();

		/** @see RenderManager::setViewSize */
		void setViewSize(int _width, int _height) override;

		/* for use with RTT, flips Y coordinate when rendering */
		void doRenderRtt(IVertexBuffer* _buffer, ITexture* _texture, size_t _count);

	/*internal:*/
		void drawOneFrame();
		bool isPixelBufferObjectSupported() const;
    unsigned int createShaderProgram(void);

	private:
		void destroyAllResources();

	private:
		IntSize mViewSize;
		bool mUpdate;
		VertexColourType mVertexFormat;
		RenderTargetInfo mInfo;
    unsigned int mProgramID;
    unsigned int mReferenceCount; // for nested rendering
    int mYScaleUniformLocation;

		typedef std::map<std::string, ITexture*> MapTexture;
		MapTexture mTextures;
		OpenGL3ImageLoader* mImageLoader;
		bool mPboIsSupported;
        
		bool mIsInitialise;
  };

} // namespace MyGUI

#endif // MYGUI_OPENGL3_RENDER_MANAGER_H_
