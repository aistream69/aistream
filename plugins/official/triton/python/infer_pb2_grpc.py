# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

from google.protobuf import empty_pb2 as google_dot_protobuf_dot_empty__pb2
import infer_pb2 as infer__pb2


class ModelInferStub(object):
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.Infer = channel.unary_unary(
                '/triton.ModelInfer/Infer',
                request_serializer=infer__pb2.InferRequest.SerializeToString,
                response_deserializer=infer__pb2.InferResponse.FromString,
                )


class ModelInferServicer(object):
    """Missing associated documentation comment in .proto file."""

    def Infer(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_ModelInferServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'Infer': grpc.unary_unary_rpc_method_handler(
                    servicer.Infer,
                    request_deserializer=infer__pb2.InferRequest.FromString,
                    response_serializer=infer__pb2.InferResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'triton.ModelInfer', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class ModelInfer(object):
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def Infer(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/triton.ModelInfer/Infer',
            infer__pb2.InferRequest.SerializeToString,
            infer__pb2.InferResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)


class GreeterStub(object):
    """Missing associated documentation comment in .proto file."""

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.GetModelRepository = channel.unary_unary(
                '/triton.Greeter/GetModelRepository',
                request_serializer=google_dot_protobuf_dot_empty__pb2.Empty.SerializeToString,
                response_deserializer=infer__pb2.ModelRepos.FromString,
                )
        self.UploadModel = channel.unary_unary(
                '/triton.Greeter/UploadModel',
                request_serializer=infer__pb2.LoadRequest.SerializeToString,
                response_deserializer=google_dot_protobuf_dot_empty__pb2.Empty.FromString,
                )
        self.RemoveModel = channel.unary_unary(
                '/triton.Greeter/RemoveModel',
                request_serializer=infer__pb2.LoadRequest.SerializeToString,
                response_deserializer=google_dot_protobuf_dot_empty__pb2.Empty.FromString,
                )


class GreeterServicer(object):
    """Missing associated documentation comment in .proto file."""

    def GetModelRepository(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def UploadModel(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def RemoveModel(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_GreeterServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'GetModelRepository': grpc.unary_unary_rpc_method_handler(
                    servicer.GetModelRepository,
                    request_deserializer=google_dot_protobuf_dot_empty__pb2.Empty.FromString,
                    response_serializer=infer__pb2.ModelRepos.SerializeToString,
            ),
            'UploadModel': grpc.unary_unary_rpc_method_handler(
                    servicer.UploadModel,
                    request_deserializer=infer__pb2.LoadRequest.FromString,
                    response_serializer=google_dot_protobuf_dot_empty__pb2.Empty.SerializeToString,
            ),
            'RemoveModel': grpc.unary_unary_rpc_method_handler(
                    servicer.RemoveModel,
                    request_deserializer=infer__pb2.LoadRequest.FromString,
                    response_serializer=google_dot_protobuf_dot_empty__pb2.Empty.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'triton.Greeter', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class Greeter(object):
    """Missing associated documentation comment in .proto file."""

    @staticmethod
    def GetModelRepository(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/triton.Greeter/GetModelRepository',
            google_dot_protobuf_dot_empty__pb2.Empty.SerializeToString,
            infer__pb2.ModelRepos.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def UploadModel(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/triton.Greeter/UploadModel',
            infer__pb2.LoadRequest.SerializeToString,
            google_dot_protobuf_dot_empty__pb2.Empty.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def RemoveModel(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/triton.Greeter/RemoveModel',
            infer__pb2.LoadRequest.SerializeToString,
            google_dot_protobuf_dot_empty__pb2.Empty.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
