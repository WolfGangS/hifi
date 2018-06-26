//
//  Avatar.h
//  interface/src/avatar
//
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Avatar_h
#define hifi_Avatar_h

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QUuid>

#include <AvatarData.h>
#include <ShapeInfo.h>
#include <render/Scene.h>
#include <graphics-scripting/Forward.h>
#include <GLMHelpers.h>


#include "Head.h"
#include "SkeletonModel.h"
#include "Rig.h"
#include "../../interface/src/ui/overlays/Overlays.h"
#include "../../interface/src/ui/overlays/Sphere3DOverlay.h"

#include <ThreadSafeValueCache.h>

namespace render {
    template <> const ItemKey payloadGetKey(const AvatarSharedPointer& avatar);
    template <> const Item::Bound payloadGetBound(const AvatarSharedPointer& avatar);
    template <> void payloadRender(const AvatarSharedPointer& avatar, RenderArgs* args);
    template <> uint32_t metaFetchMetaSubItems(const AvatarSharedPointer& avatar, ItemIDs& subItems);
}

static const float SCALING_RATIO = .05f;

extern const float CHAT_MESSAGE_SCALE;
extern const float CHAT_MESSAGE_HEIGHT;


enum ScreenTintLayer {
    SCREEN_TINT_BEFORE_LANDSCAPE = 0,
    SCREEN_TINT_BEFORE_AVATARS,
    SCREEN_TINT_BEFORE_MY_AVATAR,
    SCREEN_TINT_AFTER_AVATARS,
    NUM_SCREEN_TINT_LAYERS
};

class Texture;

using AvatarPhysicsCallback = std::function<void(uint32_t)>;

class Avatar : public AvatarData, public scriptable::ModelProvider {
    Q_OBJECT

    // This property has JSDoc in MyAvatar.h.
    Q_PROPERTY(glm::vec3 skeletonOffset READ getSkeletonOffset WRITE setSkeletonOffset)

public:
    static void setShowAvatars(bool render);
    static void setShowReceiveStats(bool receiveStats);
    static void setShowMyLookAtVectors(bool showMine);
    static void setShowOtherLookAtVectors(bool showOthers);
    static void setShowCollisionShapes(bool render);
    static void setShowNamesAboveHeads(bool show);

    explicit Avatar(QThread* thread);
    ~Avatar();

    virtual void instantiableAvatar() = 0;

    typedef render::Payload<AvatarData> Payload;

    void init();
    void updateAvatarEntities();
    void simulate(float deltaTime, bool inView);
    virtual void simulateAttachments(float deltaTime);

    virtual void render(RenderArgs* renderArgs);

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                            render::Transaction& transaction);

    void removeFromScene(AvatarSharedPointer self, const render::ScenePointer& scene,
                                render::Transaction& transaction);

    void updateRenderItem(render::Transaction& transaction);

    virtual void postUpdate(float deltaTime, const render::ScenePointer& scene);

    //setters
    void setIsLookAtTarget(const bool isLookAtTarget) { _isLookAtTarget = isLookAtTarget; }
    bool getIsLookAtTarget() const { return _isLookAtTarget; }
    //getters
    bool isInitialized() const { return _initialized; }
    SkeletonModelPointer getSkeletonModel() { return _skeletonModel; }
    const SkeletonModelPointer getSkeletonModel() const { return _skeletonModel; }
    glm::vec3 getChestPosition() const;
    const Head* getHead() const { return static_cast<const Head*>(_headData); }
    Head* getHead() { return static_cast<Head*>(_headData); }

    AABox getBounds() const;

    /// Returns the distance to use as a LOD parameter.
    float getLODDistance() const;

    virtual bool isMyAvatar() const override { return false; }

    virtual QVector<glm::quat> getJointRotations() const override;
    using AvatarData::getJointRotation;
    virtual glm::quat getJointRotation(int index) const override;
    virtual QVector<glm::vec3> getJointTranslations() const override;
    using AvatarData::getJointTranslation;
    virtual glm::vec3 getJointTranslation(int index) const override;
    virtual int getJointIndex(const QString& name) const override;
    virtual QStringList getJointNames() const override;

    /**jsdoc
     * @function MyAvatar.getDefaultJointRotation
     * @param {number} index
     * @returns {Quat} 
     */
    Q_INVOKABLE virtual glm::quat getDefaultJointRotation(int index) const;

    /**jsdoc
     * @function MyAvatar.getDefaultJointTranslation
     * @param {number} index
     * @returns {Vec3} 
     */
    Q_INVOKABLE virtual glm::vec3 getDefaultJointTranslation(int index) const;

    /**jsdoc
     * Provides read only access to the default joint rotations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file. Typically this is a T-pose.
     * @function MyAvatar.getAbsoluteDefaultJointRotationInObjectFrame
     * @param index {number} index number
     * @returns {Quat} The rotation of this joint in avatar coordinates.
     */
    Q_INVOKABLE virtual glm::quat getAbsoluteDefaultJointRotationInObjectFrame(int index) const;

    /**jsdoc
     * Provides read only access to the default joint translations in avatar coordinates.
     * The default pose of the avatar is defined by the position and orientation of all bones
     * in the avatar's model file. Typically this is a T-pose.
     * @function MyAvatar.getAbsoluteDefaultJointTranslationInObjectFrame
     * @param index {number} index number
     * @returns {Vec3} The position of this joint in avatar coordinates.
     */
    Q_INVOKABLE virtual glm::vec3 getAbsoluteDefaultJointTranslationInObjectFrame(int index) const;

    virtual glm::vec3 getAbsoluteJointScaleInObjectFrame(int index) const override;
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }

    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;
    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData) override;

    void updateDisplayNameAlpha(bool showDisplayName);
    virtual void setSessionDisplayName(const QString& sessionDisplayName) override { }; // no-op

    virtual int parseDataFromBuffer(const QByteArray& buffer) override;

    static void renderJointConnectingCone( gpu::Batch& batch, glm::vec3 position1, glm::vec3 position2,
                                                float radius1, float radius2, const glm::vec4& color);

    virtual void applyCollision(const glm::vec3& contactPoint, const glm::vec3& penetration) { }

    /**jsdoc
     * Set the offset applied to the current avatar. The offset adjusts the position that the avatar is rendered. For example, 
     * with an offset of <code>{ x: 0, y: 0.1, z: 0 }</code>, your avatar will appear to be raised off the ground slightly.
     * @function MyAvatar.setSkeletonOffset
     * @param {Vec3} offset - The skeleton offset to set.
     * @example <caption>Raise your avatar off the ground a little.</caption>
     * // Raise your avatar off the ground a little.
     * MyAvatar.setSkeletonOffset({ x: 0, y: 0.1: z: 0 });
     *
     * // Restore its offset after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.setSkeletonOffset(Vec3.ZERO);
     * }, 5000);
     */
    Q_INVOKABLE void setSkeletonOffset(const glm::vec3& offset);

    /**jsdoc
     * Get the offset applied to the current avatar. The offset adjusts the position that the avatar is rendered. For example, 
     * with an offset of <code>{ x: 0, y: 0.1, z: 0 }</code>, your avatar will appear to be raised off the ground slightly.
     * @function MyAvatar.getSkeletonOffset
     * @returns {Vec3} The current skeleton offset.
     * @example <caption>Report your avatar's current skeleton offset.</caption>
     * print(JSON.stringify(MyAvatar.getSkeletonOffset());
     */
    Q_INVOKABLE glm::vec3 getSkeletonOffset() { return _skeletonOffset; }

    virtual glm::vec3 getSkeletonPosition() const;

    /**jsdoc
     * Get the position of a joint in the current avatar.
     * @function MyAvatar.getJointPosition
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The position of the joint in world coordinates.
     */
    Q_INVOKABLE glm::vec3 getJointPosition(int index) const;

    /**jsdoc
     * Get the position of a joint in the current avatar.
     * @function MyAvatar.getJointPosition
     * @param {string} name - The name of the joint.
     * @returns {Vec3} The position of the joint in world coordinates.
     * @example <caption>Report the position of your avatar's hips.</caption>
     * print(JSON.stringify(MyAvatar.getJointPosition("Hips")));
     */
    Q_INVOKABLE glm::vec3 getJointPosition(const QString& name) const;

    /**jsdoc
     * Get the position of the current avatar's neck in world coordinates.
     * @function MyAvatar.getNeckPosition
     * @returns {Vec3} The position of the neck in world coordinates.
     * @example <caption>Report the position of your avatar's neck.</caption>
     * print(JSON.stringify(MyAvatar.getNeckPosition()));
     */
    Q_INVOKABLE glm::vec3 getNeckPosition() const;

    /**jsdoc
     * @function MyAvatar.getAcceleration
     * @returns {Vec3} 
     */
    Q_INVOKABLE glm::vec3 getAcceleration() const { return _acceleration; }

    /// Scales a world space position vector relative to the avatar position and scale
    /// \param vector position to be scaled. Will store the result
    void scaleVectorRelativeToPosition(glm::vec3 &positionToScale) const;

    void slamPosition(const glm::vec3& position);
    virtual void updateAttitude(const glm::quat& orientation) override;

    // Call this when updating Avatar position with a delta.  This will allow us to
    // _accurately_ measure position changes and compute the resulting velocity
    // (otherwise floating point error will cause problems at large positions).
    void applyPositionDelta(const glm::vec3& delta);

    virtual void rebuildCollisionShape();

    virtual void computeShapeInfo(ShapeInfo& shapeInfo);
    void getCapsule(glm::vec3& start, glm::vec3& end, float& radius);
    float computeMass();

    void setPositionViaScript(const glm::vec3& position) override;
    void setOrientationViaScript(const glm::quat& orientation) override;


    /**jsdoc
     * @function MyAvatar.getParentID
     * @returns {Uuid} 
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual const QUuid getParentID() const override { return SpatiallyNestable::getParentID(); }

    /**jsdoc
     * @function MyAvatar.setParentID
     * @param {Uuid} parentID
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual void setParentID(const QUuid& parentID) override;

    /**jsdoc
     * @function MyAvatar.getParentJointIndex
     * @returns {number} 
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual quint16 getParentJointIndex() const override { return SpatiallyNestable::getParentJointIndex(); }

    /**jsdoc
     * @function MyAvatar.setParentJointIndex
     * @param {number} parentJointIndex
     */
    // This calls through to the SpatiallyNestable versions, but is here to expose these to JavaScript.
    Q_INVOKABLE virtual void setParentJointIndex(quint16 parentJointIndex) override;


    /**jsdoc
     * Returns an array of joints, where each joint is an object containing name, index, and parentIndex fields.
     * @function MyAvatar.getSkeleton
     * @returns {MyAvatar.SkeletonJoint[]} A list of information about each joint in this avatar's skeleton.
     */
    /**jsdoc
     * Information about a single joint in an Avatar's skeleton hierarchy.
     * @typedef {object} MyAvatar.SkeletonJoint
     * @property {string} name - Joint name.
     * @property {number} index - Joint index.
     * @property {number} parentIndex - Index of this joint's parent (-1 if no parent).
     */
    Q_INVOKABLE QList<QVariant> getSkeleton();

    // NOT thread safe, must be called on main thread.
    glm::vec3 getUncachedLeftPalmPosition() const;
    glm::quat getUncachedLeftPalmRotation() const;
    glm::vec3 getUncachedRightPalmPosition() const;
    glm::quat getUncachedRightPalmRotation() const;

    uint64_t getLastRenderUpdateTime() const { return _lastRenderUpdateTime; }
    void setLastRenderUpdateTime(uint64_t time) { _lastRenderUpdateTime = time; }

    void animateScaleChanges(float deltaTime);
    void setTargetScale(float targetScale) override;
    float getTargetScale() const { return _targetScale; }

    /**jsdoc
     * @function MyAvatar.getSimulationRate
     * @param {string} [rateName=""]
     * @returns {number} 
     */
    Q_INVOKABLE float getSimulationRate(const QString& rateName = QString("")) const;

    bool hasNewJointData() const { return _hasNewJointData; }

    float getBoundingRadius() const;
    AABox getRenderBounds() const; // THis call is accessible from rendering thread only to report the bounding box of the avatar during the frame.

    void addToScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    void ensureInScene(AvatarSharedPointer self, const render::ScenePointer& scene);
    bool isInScene() const { return render::Item::isValidID(_renderItemID); }
    render::ItemID getRenderItemID() { return _renderItemID; }
    bool isMoving() const { return _moving; }

    void setPhysicsCallback(AvatarPhysicsCallback cb);
    void addPhysicsFlags(uint32_t flags);
    bool isInPhysicsSimulation() const { return _physicsCallback != nullptr; }

    void fadeIn(render::ScenePointer scene);
    void fadeOut(render::ScenePointer scene, KillAvatarReason reason);
    bool isFading() const { return _isFading; }
    void updateFadingStatus(render::ScenePointer scene);

    // JSDoc is in AvatarData.h.
    Q_INVOKABLE virtual float getEyeHeight() const override;

    // returns eye height of avatar in meters, ignoring avatar scale.
    // if _targetScale is 1 then this will be identical to getEyeHeight.
    virtual float getUnscaledEyeHeight() const override;

    // returns true, if an acurate eye height estimage can be obtained by inspecting the avatar model skeleton and geometry,
    // not all subclasses of AvatarData have access to this data.
    virtual bool canMeasureEyeHeight() const override { return true; }


    virtual float getModelScale() const { return _modelScale; }
    virtual void setModelScale(float scale) { _modelScale = scale; }
    virtual glm::vec3 scaleForChildren() const override { return glm::vec3(getModelScale()); }

    virtual void setAvatarEntityDataChanged(bool value) override;

    // Show hide the model representation of the avatar
    virtual void setEnableMeshVisible(bool isEnabled);
    virtual bool getEnableMeshVisible() const;

    void addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName) override;
    void removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName) override;

    virtual scriptable::ScriptableModelBase getScriptableModel() override;
    
    void updateOrbPosition();
    std::shared_ptr<Sphere3DOverlay> _purpleOrbMeshPlaceholder{ nullptr };
    OverlayID _purpleOrbMeshPlaceholderID{ UNKNOWN_OVERLAY_ID };
public slots:

    // FIXME - these should be migrated to use Pose data instead
    // thread safe, will return last valid palm from cache

    /**jsdoc
     * Get the position of the left palm in world coordinates.
     * @function MyAvatar.getLeftPalmPosition
     * @returns {Vec3} The position of the left palm in world coordinates.
     * @example <caption>Report the position of your avatar's left palm.</caption>
     * print(JSON.stringify(MyAvatar.getLeftPalmPosition()));
     */
    glm::vec3 getLeftPalmPosition() const;

    /**jsdoc
     * Get the rotation of the left palm in world coordinates.
     * @function MyAvatar.getLeftPalmRotation
     * @returns {Vec3} The rotation of the left palm in world coordinates.
     * @example <caption>Report the rotation of your avatar's left palm.</caption>
     * print(JSON.stringify(MyAvatar.getLeftPalmRotation()));
     */
    glm::quat getLeftPalmRotation() const;
    /**jsdoc
     * Get the position of the right palm in world coordinates.
     * @function MyAvatar.getRightPalmPosition
     * @returns {Vec3} The position of the right palm in world coordinates.
     * @example <caption>Report the position of your avatar's right palm.</caption>
     * print(JSON.stringify(MyAvatar.getRightPalmPosition()));
     */
    glm::vec3 getRightPalmPosition() const;

    /**jsdoc
     * Get the rotation of the right palm in world coordinates.
     * @function MyAvatar.getRightPalmRotation
     * @returns {Vec3} The rotation of the right palm in world coordinates.
     * @example <caption>Report the rotation of your avatar's right palm.</caption>
     * print(JSON.stringify(MyAvatar.getRightPalmRotation()));
     */
    glm::quat getRightPalmRotation() const;

    // hooked up to Model::setURLFinished signal
    void setModelURLFinished(bool success);

    /**jsdoc
     * @function MyAvatar.rigReady
     * @returns {Signal} 
     */
    // Hooked up to Model::rigReady signal
    void rigReady();

    /**jsdoc
     * @function MyAvatar.rigReset
     * @returns {Signal} 
     */
    // Jooked up to Model::rigReset signal
    void rigReset();

protected:
    float getUnscaledEyeHeightFromSkeleton() const;
    void buildUnscaledEyeHeightCache();
    void clearUnscaledEyeHeightCache();
    virtual const QString& getSessionDisplayNameForTransport() const override { return _empty; } // Save a tiny bit of bandwidth. Mixer won't look at what we send.
    QString _empty{};
    virtual void maybeUpdateSessionDisplayNameFromTransport(const QString& sessionDisplayName) override { _sessionDisplayName = sessionDisplayName; } // don't use no-op setter!

    SkeletonModelPointer _skeletonModel;

    void invalidateJointIndicesCache() const;
    void withValidJointIndicesCache(std::function<void()> const& worker) const;
    mutable QHash<QString, int> _modelJointIndicesCache;
    mutable QReadWriteLock _modelJointIndicesCacheLock;
    mutable bool _modelJointsCached { false };

    glm::vec3 _skeletonOffset;
    std::vector<std::shared_ptr<Model>> _attachmentModels;
    std::vector<bool> _attachmentModelsTexturesLoaded;
    std::vector<std::shared_ptr<Model>> _attachmentsToRemove;
    std::vector<std::shared_ptr<Model>> _attachmentsToDelete;

    float _bodyYawDelta { 0.0f };  // degrees/sec

    // These position histories and derivatives are in the world-frame.
    // The derivatives are the MEASURED results of all external and internal forces
    // and are therefore READ-ONLY --> motion control of the Avatar is NOT obtained
    // by setting these values.
    // Floating point error prevents us from accurately measuring velocity using a naive approach
    // (e.g. vel = (pos - lastPos)/dt) so instead we use _positionDeltaAccumulator.
    glm::vec3 _positionDeltaAccumulator;
    glm::vec3 _lastVelocity;
    glm::vec3 _acceleration;
    glm::vec3 _angularVelocity;
    glm::vec3 _lastAngularVelocity;
    glm::vec3 _angularAcceleration;
    glm::quat _lastOrientation;

    glm::vec3 _worldUpDirection { Vectors::UP };
    bool _moving { false }; ///< set when position is changing

    // protected methods...
    bool isLookingAtMe(AvatarSharedPointer avatar) const;
    void relayJointDataToChildren();

    void fade(render::Transaction& transaction, render::Transition::Type type);

    glm::vec3 getBodyRightDirection() const { return getWorldOrientation() * IDENTITY_RIGHT; }
    glm::vec3 getBodyUpDirection() const { return getWorldOrientation() * IDENTITY_UP; }
    void measureMotionDerivatives(float deltaTime);

    float getSkeletonHeight() const;
    float getHeadHeight() const;
    float getPelvisFloatingHeight() const;
    glm::vec3 getDisplayNamePosition() const;

    Transform calculateDisplayNameTransform(const ViewFrustum& view, const glm::vec3& textPosition) const;
    void renderDisplayName(gpu::Batch& batch, const ViewFrustum& view, const glm::vec3& textPosition) const;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const;
    virtual void fixupModelsInScene(const render::ScenePointer& scene);

    virtual void updatePalms();

    render::ItemID _renderItemID{ render::Item::INVALID_ITEM_ID };

    ThreadSafeValueCache<glm::vec3> _leftPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _leftPalmRotationCache { glm::quat() };
    ThreadSafeValueCache<glm::vec3> _rightPalmPositionCache { glm::vec3() };
    ThreadSafeValueCache<glm::quat> _rightPalmRotationCache { glm::quat() };

    // Some rate tracking support
    RateCounter<> _simulationRate;
    RateCounter<> _simulationInViewRate;
    RateCounter<> _skeletonModelSimulationRate;
    RateCounter<> _jointDataSimulationRate;

protected:
    class AvatarEntityDataHash {
    public:
        AvatarEntityDataHash(uint32_t h) : hash(h) {};
        uint32_t hash { 0 };
        bool success { false };
    };

    using MapOfAvatarEntityDataHashes = QMap<QUuid, AvatarEntityDataHash>;
    MapOfAvatarEntityDataHashes _avatarEntityDataHashes;

    uint64_t _lastRenderUpdateTime { 0 };
    int _leftPointerGeometryID { 0 };
    int _rightPointerGeometryID { 0 };
    int _nameRectGeometryID { 0 };
    bool _initialized { false };
    bool _isLookAtTarget { false };
    bool _isAnimatingScale { false };
    bool _mustFadeIn { false };
    bool _isFading { false };
    bool _reconstructSoftEntitiesJointMap { false };
    float _modelScale { 1.0f };

    static int _jointConesID;

    int _voiceSphereID;

    AvatarPhysicsCallback _physicsCallback { nullptr };

    float _displayNameTargetAlpha { 1.0f };
    float _displayNameAlpha { 1.0f };

    ThreadSafeValueCache<float> _unscaledEyeHeightCache { DEFAULT_AVATAR_EYE_HEIGHT };

    std::unordered_map<std::string, graphics::MultiMaterial> _materials;
    std::mutex _materialsLock;

    void processMaterials();

    AABox _renderBound;
    bool _isMeshVisible{ true };
    bool _needMeshVisibleSwitch{ true };
};

#endif // hifi_Avatar_h
