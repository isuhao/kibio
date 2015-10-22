// =============================================================================
//
// Copyright (c) 2014 Christopher Baker <http://christopherbaker.net>
//               2015 Brannon Dorsey <http://brannondorsey.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "Project.h"
#include "Poco/FileStream.h"
#include "Poco/UTF8String.h"


namespace Kibio {


const std::string Project::FILE_EXTENSION = ".kibio";


Project::Project(AbstractApp& parent):
    _parent(parent),
    _isLoaded(false),
    _maskBrushEnabled(false),
    _transform(NONE)
{
    ofRegisterDragEvents(this);
    ofRegisterKeyEvents(this);
    ofRegisterMouseEvents(this);
}


Project::~Project()
{
    ofUnregisterDragEvents(this);
    ofUnregisterKeyEvents(this);
    ofUnregisterMouseEvents(this);
}


void Project::update()
{
    std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.begin();

    while (iter != _layers.end())
    {
        if ((*iter))
        {
            (*iter)->update();
        }

        ++iter;
    }
}


void Project::draw()
{
    std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.begin();

    while (iter != _layers.end())
    {
        if ((*iter))
        {
            (*iter)->draw();
        }

        ++iter;
    }

    if (_dragging)
    {
        ofPoint mouse(ofGetMouseX(), ofGetMouseY());
        ofPushStyle();

        ofSetColor(255, 255, 0);

        if (_transform == TRANSLATE)
        {
            _dragging->drawTranslatePreview(mouse, _dragStart);
        }
        else if (_transform == ROTATE)
        {
            _dragging->drawRotatePreview(mouse, _dragStart);
        }
        else if (_transform == SCALE)
        {
            _dragging->drawScalePreview(mouse, _dragStart);
        }

        ofPopStyle();
    }
}


void Project::dragEvent(ofDragInfo& dragInfo)
{
    if (_parent.getMode() != AbstractApp::EDIT)
    {
        return;
    }

    std::vector<std::string>::const_iterator fileIter = dragInfo.files.begin();

    if (!dragInfo.files.empty())
    {
        Poco::Path path(dragInfo.files[0]);
        Poco::Path cleanPath(path.parent(), Poco::UTF8::toLower(path.getFileName()));
        Poco::Net::MediaType mediaType = ofx::MediaTypeMap::getDefault()->getMediaTypeForPath(cleanPath).toString();

        if (mediaType.matches("video"))
        {
            Poco::Path relativePath = path;

            if (makeRelativeToProjectFolder(relativePath))
            {
                newLayerWithVideoAtPoint(relativePath, dragInfo.position);
            }
            else
            {
                std::string msg = relativePath.getFileName() + "  was not added to the project because it is not located in the project folder.";
                ofSystemAlertDialog(msg);
                ofLogError("Project::dragEvent") << msg;
            }

        }
        else if (mediaType.matches("image"))
        {
            Poco::Path relativePath = path;

            if (makeRelativeToProjectFolder(relativePath))
            {
               setMaskForLayerAtPoint(relativePath, dragInfo.position);
            }
            else
            {
                std::string msg = relativePath.getFileName() + "  was not added to the project because it is not located in the project folder.";
                ofSystemAlertDialog(msg);
                ofLogError("Project::dragEvent") << msg;
            }
        }
        else
        {
            ofLogError("Project::dragEvent") << "File must be a video or image: " << path.toString() << " : " << mediaType.toString();
            // TODO: Perhaps this isn't the appropriate place for this
            ofSystemAlertDialog("Unsupported file type detected.");
        }
    }
}


bool Project::makeRelativeToProjectFolder(Poco::Path& path) const
{
    if (isFileInProjectFolder(path))
    {
        Poco::Path projectPath = getPath();
        Poco::Path otherPath = path;

        projectPath.makeAbsolute();
        otherPath.makeAbsolute();

        while (projectPath[0] == otherPath[0])
        {
            projectPath.popFrontDirectory();
            otherPath.popFrontDirectory();
        }

        path = Poco::Path(); // Make relative path.

        // Transfer directories.
        for (std::size_t i = 0; i < otherPath.depth(); ++i)
        {
            path.pushDirectory(otherPath.directory(i));
        }

        // Set the file name.
        path.setFileName(otherPath.getFileName());

        return true;
    }
    else
    {
        return false;
    }
}


bool Project::isFileInProjectFolder(const Poco::Path& path) const
{
    Poco::Path projectPath = getPath();
    Poco::Path otherPath = path;

    projectPath.makeAbsolute();
    otherPath.makeAbsolute();

    if (projectPath.depth() > otherPath.depth()) return false;

    for (std::size_t i = 0; i < projectPath.depth(); ++i)
    {
        if (i < otherPath.depth())
        {
            std::string parent = projectPath.directory(i);
            std::string other = otherPath.directory(i);

            if (parent != other)
            {
                return false;
            }
        }
    }

    return true;
}


void Project::newLayerWithVideoAtPoint(const Poco::Path& videoPath, const ofPoint& point)
{
    if (_parent.getMode() != AbstractApp::EDIT)
    {
        return;
    }

    std::shared_ptr<Layer> layer(new Layer(*this));

    if (!layer->loadVideo(videoPath.toString()))
    {
        ofLogError("Project::newLayerWithVideoAtPoint") << "Layer not created: " << videoPath.toString();
    }
    else
    {
        _layers.push_back(layer);
    }
}


void Project::setMaskForLayerAtPoint(const Poco::Path& maskPath, const ofPoint& point)
{
    if (_parent.getMode() != AbstractApp::EDIT) return;

    std::shared_ptr<Layer> layer = getLayerAtPoint(point);

    if (layer)
    {
        if (!layer->loadMask(maskPath.toString()))
        {
            ofLogError("Project::setMaskForLayerAtPoint") << "Unable to load mask.";
        }
    }
    else
    {
        ofLogError("Project::setMaskForLayerAtPoint") << "No layer at point: " << point;
    }
}


void Project::deleteLayerAtPoint(const ofPoint& point)
{
    if (_parent.getMode() != AbstractApp::EDIT)
    {
        return;
    }

    std::shared_ptr<Layer> layer = getLayerAtPoint(point);

    if (layer)
    {
        _layers.erase(std::find(_layers.begin(),
                                _layers.end(),
                                layer));
        
        if (_lastSelectedLayer && _lastSelectedLayer->getId() == layer->getId())
        {
            _lastSelectedLayer.reset();
        }
    }
    else
    {
        ofLogError("Project::deleteLayerAtPoint") << "No layer at point: " << point;
    }
}


void Project::clearMaskAtPoint(const ofPoint& point)
{
    if (_parent.getMode() != AbstractApp::EDIT)
    {
        return;
    }

    std::shared_ptr<Layer> layer = getLayerAtPoint(point);

    if (layer)
    {
        layer->clearMask();
    }
    else
    {
        ofLogError("Project::clearMaskAtPoint") << "No mask at point: " << point;
    }
}
    
void Project::shiftLayer(LayerShift shift)
{
    if (_lastSelectedLayer)
    {
        shiftLayer(_lastSelectedLayer, shift);
    }
}
    
void Project::shiftLayer(Layer::SharedPtr layer, LayerShift shift)
{
    if (layer)
    {
        
        if (shift == LAYER_SHIFT_UP)
        {

            std::deque<std::shared_ptr<Layer> >::iterator iter = _layers.begin();
            
            while (iter != _layers.end() - 1)
            {
                if ((*iter) && (*iter)->getId() == layer->getId())
                {
                    std::iter_swap(iter, iter + 1);
                    break;
                }
                
                ++iter;
            }
        }
        else if (shift == LAYER_SHIFT_DOWN)
        {
            std::deque<std::shared_ptr<Layer> >::iterator iter = _layers.begin() + 1;
            
            while (iter != _layers.end())
            {
                if ((*iter) && (*iter)->getId() == layer->getId())
                {
                    std::iter_swap(iter, iter - 1);
                    break;
                }
                
                ++iter;
            }
        }
        if (shift == LAYER_SHIFT_TOP)
        {
            // check if this is already the top layer, if so ignore
            if (layer->getId() != _layers[_layers.size() - 1]->getId())
            {
                std::deque<std::shared_ptr<Layer> >::iterator iter = _layers.begin();
                
                while (iter != _layers.end())
                {
                    if ((*iter) && (*iter)->getId() == layer->getId())
                    {
                        _layers.erase(iter);
                        _layers.push_back(layer);
                        break;
                    }
                    
                    ++iter;
                }
            }
        }
        else if (shift == LAYER_SHIFT_BOTTOM)
        {
            // check if this is already the bottom layer, if so ignore
            if (layer->getId() != _layers[0]->getId())
            {
                std::deque<std::shared_ptr<Layer> >::iterator iter = _layers.begin();
                
                while (iter != _layers.end())
                {
                    if ((*iter) && (*iter)->getId() == layer->getId())
                    {
                        _layers.erase(iter);
                        _layers.push_front(layer);
                        break;
                    }
                    
                    ++iter;
                }
            }
        }
    }
}

std::shared_ptr<Layer> Project::getLayerAtPoint(const ofPoint& point) const
{
    std::shared_ptr<Layer> empty;

    std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.end() - 1;

    while (iter != _layers.begin() - 1)
    {
        if ((*iter))
        {
            if ((*iter)->hitTest(point))
            {
                return *iter;
            }
        }

        --iter;
    }

    return empty;
}


bool Project::load(const std::string& name)
{
    _path = Poco::Path(_parent.getUserProjectsPath(),
                       Poco::Path::forDirectory(name));

    try
    {
        Poco::Path settingsPath(_path, name + FILE_EXTENSION);

        Poco::FileInputStream fis(settingsPath.toString());

        Json::Value json;
        Json::Reader reader;

        if (reader.parse(fis, json))
        {
            fromJSON(json, *this);
            fis.close();
        }
        else
        {
            fis.close();
            throw Poco::Exception(reader.getFormattedErrorMessages(), settingsPath.toString());
        }

        _isLoaded = true;
    }
    catch (const Poco::Exception& exc)
    {
        ofLogError("Project::load") << exc.displayText();
        _isLoaded = false;
    }

    return _isLoaded;
}


bool Project::create(const std::string& name, const std::string& templateDir)
{
    // template directory folder
    Poco::File tempDir(ofToDataPath(templateDir));

    ofLogVerbose("Project::create") << "Copying template directory from \"" << tempDir.path() << "\"";

    if (tempDir.exists() && tempDir.isDirectory())
    {
        Poco::Path newProjectPath(_parent.getUserProjectsPath(), name);
        Poco::File newProjectFolder(newProjectPath);

        if (newProjectFolder.exists())
        {
            ofLogError("Project::create") << "\"" << newProjectPath.toString() << "\" already exists";
            return false;
        }

        try
        {
            tempDir.copyTo(newProjectPath.toString());

            // rename the project file
            Poco::Path projectFilePath(newProjectPath, "TemplateProject" + Project::FILE_EXTENSION);
            Poco::File projectFile(projectFilePath);

            if (!projectFile.exists()) {
                ofLogError("Project::create") << "Project file \"" << projectFile.path() << "\" does not exist";
                return false;
            }

            Poco::Path newProjectFilePath(projectFilePath.parent(), name + Project::FILE_EXTENSION);

            try
            {

                projectFile.renameTo(newProjectFilePath.toString());

                ofLogNotice("Project::create") << "Project \"" << name << "\" created" ;

                return true;
            }
            catch (const Poco::Exception& exc)
            {
                ofLogError("Project::create") << exc.displayText();
                return false;
            }
        }
        catch (const Poco::Exception& exc)
        {
            ofLogError("Project::create") << exc.displayText();
            return false;
        }

    }
    else
    {
        ofLogError("Project::create")
            << "Template Directory \"" << templateDir << "\" does not exist or is not a directory" ;

        return false;
    }

}

bool Project::save()
{
    Poco::Path settingsPath(_path, getName() + FILE_EXTENSION);

    try
    {
        std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.begin();

        while (iter != _layers.end())
        {
            if ((*iter))
            {
                (*iter)->saveMask();
            }

            ++iter;
        }

        Poco::FileOutputStream fos(settingsPath.toString());
        Json::StyledWriter writer;
        fos << writer.write(toJSON(*this));
        fos.close();
        return true;
    }
    catch (const Poco::Exception& exc)
    {
        ofLogError("Project::save") << exc.displayText();
        return false;
    }
}

bool Project::saveAs(const std::string& name)
{
    Poco::Path newProjectFolderPath(_path.parent(), name);
    Poco::File newProjectFolderFile(newProjectFolderPath);

    if (newProjectFolderFile.exists())
    {
        ofLogError("Project::saveAs") << newProjectFolderPath.toString() << " already exists";
        return false;
    }

    Poco::File oldProjectFolderFile(_path);

    try
    {
        oldProjectFolderFile.copyTo(newProjectFolderPath.toString());

        Poco::Path settingsFilePath(newProjectFolderPath, getName() + FILE_EXTENSION);
        Poco::Path newSettingsFilePath(newProjectFolderPath, name + FILE_EXTENSION);
        Poco::File settingsFile(settingsFilePath);

        try
        {
            settingsFile.renameTo(newSettingsFilePath.toString());
            return true;
        }
        catch (const Poco::Exception& exc)
        {
            ofLogError("Project::saveAs") << "Could not copy " << settingsFile.path()
            << " to " << newProjectFolderPath.toString() << name << FILE_EXTENSION << " " << exc.displayText();
            return false;
        }
    }
    catch (const Poco::Exception& exc)
    {
        ofLogError("Project::saveAs") << "Could not copy " << oldProjectFolderFile.path()
            << " to " << newProjectFolderPath.toString() << " " << exc.displayText();
        return false;
    }
}


bool Project::isLoaded() const
{
    return _isLoaded;
}


bool Project::isCornerHovered(const ofPoint& point) const
{
    std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.begin();

    while (iter != _layers.end())
    {
        if ((*iter))
        {
            if ((*iter)->getHoveredCorner(point)) return true;
        }

        ++iter;
    }

    return false;
}


void Project::enableMaskBrush()
{
    _maskBrushEnabled = true;
}


void Project::disableMaskBrush()
{
    _maskBrushEnabled = false;
}


bool Project::isMaskBrushEnabled()
{
    return _maskBrushEnabled;
}


std::string Project::getName() const
{
    return _path.directory(_path.depth() - 1);
}


Poco::Path Project::getPath() const
{
    return _path;
}

void Project::setTransform(TransformType type)
{
    _transform = type;
}


//std::string Project::toDebugString(const ofx::Media::AVMediaInfo& info)
//{
//    std::stringstream ss;
//
//    ss << "filename:" << " " << info.path.getFileName() << endl;
//    ss << setw(10) << "Metadata:" << endl;
//
//    Poco::Net::NameValueCollection::ConstIterator iter = info.metadata.begin();
//    while(iter != info.metadata.end())
//    {
//        ss << setw(20) << iter->first << "=" << iter->second << endl;
//        ++iter;
//    }
//
//    for(size_t i = 0; i < info.streams.size(); i++)
//    {
//        ss << endl;
//        ss << "stream (" << ofToString(i+1) << "/" << info.streams.size() << ")" << endl;
//        ss << "=========================================================================" << endl;
//
//        ofx::Media::AVStreamInfo stream = info.streams[i];
//
//        if(stream.codecType == AVMEDIA_TYPE_VIDEO)
//        {
//            ss << setw(20) << "type:" << " " << "VIDEO STREAM" << endl;
//            ss << setw(20) << "width:" << " " << stream.videoWidth << endl;
//            ss << setw(20) << "height:" << " " << stream.videoHeight << endl;
//            ss << setw(20) << "decoded format:" << " " << stream.videoDecodedFormat << endl;
//
//        }
//        else if(stream.codecType == AVMEDIA_TYPE_AUDIO)
//        {
//            ss << setw(20) << "type:" << " " << "AUDIO STREAM" << endl;
//            ss << setw(20) << "num channels:" << " " << stream.audioNumChannels << endl;
//            ss << setw(20) << "bits / sample:" << " " << stream.audioBitsPerSample << endl;
//            ss << setw(20) << "sample rate:" << " " << stream.audioSampleRate << endl;
//
//        }
//        else
//        {
//        }
//
//        ss << setw(20) << "codec:" << " " << stream.codecName << " [" << stream.codecLongName << "]" << endl;
//        ss << setw(20) << "codec tag:" << " " << stream.codecTag << endl;
//        ss << setw(20) << "stream codec tag:" << " " << stream.streamCodecTag << endl;
//        ss << setw(20) << "profile:" << " " << stream.codecProfile << endl;
//        ss << endl;
//
//        ss << setw(20) << "avg. bitrate:" << " " << stream.averageBitRate << endl;
//        ss << setw(20) << "avg. framerate:" << " " << (double)stream.averageFrameRate.num / stream.averageFrameRate.den << endl;
//
//        ss << setw(20) << "Metadata:" << endl;
//
//        Poco::Net::NameValueCollection::ConstIterator iter = stream.metadata.begin();
//        while(iter != stream.metadata.end())
//        {
//            ss << setw(30) << iter->first << "=" << iter->second << endl;
//            ++iter;
//        }
//
//    }
//
//    return ss.str();
//}


Json::Value Project::toJSON(const Project& object)
{
    Json::Value json;

    std::deque<std::shared_ptr<Layer> >::const_iterator iter = object._layers.begin();

    while (iter != object._layers.end())
    {
        json["layers"].append(Layer::toJSON(*(*iter)));
        ++iter;
    }

    return json;
}


bool Project::fromJSON(const Json::Value& json, Project& object)
{
    if (json.isMember("layers"))
    {
        const Json::Value& layers = json["layers"];

        for (Json::ArrayIndex i = 0; i < layers.size(); ++i)
        {
            const Json::Value& layer = layers[i];

            std::shared_ptr<Layer> pLayer(new Layer(object));

            if (!Layer::fromJSON(layer, *pLayer))
            {
                ofLogError("Project::fromJSON") << "Unable to load layer, skipping.";
            }
            else
            {
                object._layers.push_back(pLayer);
            }

        }
    }

    return true;
}


void Project::keyPressed(ofKeyEventArgs& key)
{
#if defined(TARGET_OSX)
    int modifier = OF_KEY_COMMAND;
#else
    int modifier = OF_KEY_CONTROL;
#endif

    if (ofGetKeyPressed(modifier))
    {
        if ('x' == key.key)
        {
            std::deque<std::shared_ptr<Layer> >::const_iterator iter = _layers.begin();

            while (iter != _layers.end())
            {
                if ((*iter))
                {
                    if ((*iter)->_video)
                    {
                        (*iter)->_video->setPosition(0);
                    }
                }

                ++iter;
            }
        }
        else if (OF_KEY_DEL == key.key || OF_KEY_BACKSPACE == key.key)
        {
            ofPoint mouse(ofGetMouseX(), ofGetMouseY());
            clearMaskAtPoint(mouse);
        }
        else if (key.key == ']')
        {
            if (ofGetKeyPressed(OF_KEY_SHIFT))
            {
                shiftLayer(LAYER_SHIFT_TOP);
            }
            else
            {
                shiftLayer(LAYER_SHIFT_UP);
            }
        }
        else if (key.key == '[')
        {
            if (ofGetKeyPressed(OF_KEY_SHIFT))
            {
                shiftLayer(LAYER_SHIFT_BOTTOM);
            }
            else
            {
                shiftLayer(LAYER_SHIFT_DOWN);
            }
        }

    }
    else if (OF_KEY_DEL == key.key || OF_KEY_BACKSPACE == key.key)
    {
        ofPoint mouse(ofGetMouseX(), ofGetMouseY());
        deleteLayerAtPoint(mouse);
    }

}


void Project::keyReleased(ofKeyEventArgs& key)
{
}


void Project::mouseMoved(ofMouseEventArgs& mouse)
{
}


void Project::mouseDragged(ofMouseEventArgs& mouse)
{
}


void Project::mousePressed(ofMouseEventArgs& mouse)
{
    if (_parent.getMode() != AbstractApp::EDIT)
    {
        return;
    }

    std::shared_ptr<Layer> layer = getLayerAtPoint(mouse);

    if (layer && !isCornerHovered(mouse))
    {
        _dragging = layer;
        _dragStart = mouse;
        _lastSelectedLayer = layer;
        shiftLayer(layer, LAYER_SHIFT_TOP);
    }
}

void Project::mouseReleased(ofMouseEventArgs& mouse)
{
    if (_dragging)
    {
        ofPoint dragEnd = mouse;
        ofPoint delta = dragEnd - _dragStart;

        if (_transform == TRANSLATE)
        {
            _dragging->translate(delta);
        }
        else if (_transform == ROTATE)
        {
            // these MUST be ofVec2f not ofPoint to allow ofVec2f::angle() to return
            // negative values. This may be a bug in ofPoint.
            ofVec2f centroid = _dragging->getCentroid();
            ofVec2f deltaVec = mouse - centroid;

            int angle = -(deltaVec.angle(_dragStart - centroid));
            _dragging->rotate(angle);

        }
        else if (_transform == SCALE)
        {
            ofVec2f centroid = _dragging->getCentroid();
            ofVec2f deltaVec = mouse - centroid;
            float mult = centroid.distance(mouse) / centroid.distance(_dragStart);
            _dragging->scale(mult);
        }

        _dragging.reset();
    }
}

void Project::mouseScrolled(ofMouseEventArgs& mouse)
{
}


} // namespace Kibio
