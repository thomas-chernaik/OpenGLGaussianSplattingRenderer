import numpy as np
# A function that takes a a 3D covariance matrix and returns the angles and scales of the ellipsoid
def covariance_to_angles_and_scales(cov):
    # Compute the eigenvalues and eigenvectors
    eigvals, eigvecs = np.linalg.eigh(cov)
    # Sort the eigenvalues and eigenvectors in decreasing order
    order = eigvals.argsort()[::-1]
    eigvals = eigvals[order]
    eigvecs = eigvecs[:,order]
    # Compute the angles and scales
    angleQuaternion = np.zeros((4,))
    angleQuaternion[0] = np.sqrt(1 + eigvecs[0,0] + eigvecs[1,1] + eigvecs[2,2]) / 2
    angleQuaternion[1] = (eigvecs[2,1] - eigvecs[1,2]) / (4 * angleQuaternion[0])
    angleQuaternion[2] = (eigvecs[0,2] - eigvecs[2,0]) / (4 * angleQuaternion[0])
    angleQuaternion[3] = (eigvecs[1,0] - eigvecs[0,1]) / (4 * angleQuaternion[0])
    scales = np.sqrt(eigvals)
    # Print the results
    print("Quaternions:", angleQuaternion)
    print("Scales:", scales)

# A function that takes a quaternion and scales and returns the 3D covariance matrix
def angles_and_scales_to_covariance(quat, scales):
    # Compute the rotation matrix from the quaternion
    x, y, z, w = quat
    R = np.array([
        [1 - 2*y**2 - 2*z**2, 2*x*y - 2*z*w, 2*x*z + 2*y*w],
        [2*x*y + 2*z*w, 1 - 2*x**2 - 2*z**2, 2*y*z - 2*x*w],
        [2*x*z - 2*y*w, 2*y*z + 2*x*w, 1 - 2*x**2 - 2*y**2]
    ])
    # Compute the covariance matrix
    cov = R @ np.diag(scales**2) @ R.T
    # Print the results
    print("Covariance:", cov)

def quaternion_to_rotation_matrix(quat):
    x, y, z, w = quat
    R = np.array([
        [1 - 2*y**2 - 2*z**2, 2*x*y - 2*z*w, 2*x*z + 2*y*w],
        [2*x*y + 2*z*w, 1 - 2*x**2 - 2*z**2, 2*y*z - 2*x*w],
        [2*x*z - 2*y*w, 2*y*z + 2*x*w, 1 - 2*x**2 - 2*y**2]
    ])
    return R

cov = np.array([
    [0.19140625, 0.0, 0.0],
    [0.0, 0.00781324971, 0.0005859],
    [0.0, 0.0005859, 0.00984299742]
])
cov = np.array([[0.19000002, 0.10392303, 0.],
                [0.10392303, 0.06999998, 0.],
                [0., 0., 0.01]])
covariance_to_angles_and_scales(cov)

quat = np.array([ 0, 0, 0.258819, 0.9659258 ])
print(f"Quat: {quaternion_to_rotation_matrix(quat)}")
scales = np.array([0.5, 0.1, 0.1])
angles_and_scales_to_covariance(quat, scales)