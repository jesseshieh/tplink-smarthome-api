/* eslint-disable no-param-reassign */
import { Client, Plug } from '../../src';
import { AnyDevice } from '../../src/client';
import { isObjectLike } from '../../src/utils';

export type TestDevice = {
  name: string;
  model: string;
  hardwareVersion?: string;
  isSimulated: boolean;
  deviceOptions?: Parameters<Client['getDevice']>[0];
  deviceType?: 'bulb' | 'plug';
  mac?: string;
  getDevice?: (
    deviceOptions?: Parameters<Client['getDevice']>[0],
    sendOptions?: Parameters<Client['getDevice']>[1]
  ) => ReturnType<Client['getDevice']>;
  parent?: TestDevice;
  children?: TestDevice[];
  childId?: string;
  getOtherChildren?: () => TestDevice[];
  getOtherChildrenState?: () => Promise<
    Array<{
      childId: string;
      relayState: boolean;
      alias: string;
    }>
  >;
};

export function likeTestDevice(candidate: unknown): candidate is TestDevice {
  return (
    isObjectLike(candidate) &&
    'name' in candidate &&
    'model' in candidate &&
    'isSimulated' in candidate
  );
}

const testDevices: TestDevice[] = [];

export function createTestDevice(
  device: AnyDevice,
  client: Client,
  {
    name,
    model,
    isSimulated,
    hardwareVersion,
    parent,
    childId,
  }: {
    name: string;
    model: string;
    isSimulated: boolean;
    hardwareVersion: string;
    parent: TestDevice;
    childId: string;
  }
): TestDevice {
  const testDevice: TestDevice = {
    name,
    model,
    hardwareVersion,
    isSimulated,
    parent,
    childId,
  };

  // eslint-disable-next-line @typescript-eslint/no-use-before-define
  return testDeviceDecorator(testDevice, device, client, { parent, childId });
}

export function testDeviceDecorator(
  testDevice: TestDevice,
  device: AnyDevice,
  client: Client,
  {
    parent,
    childId,
  }: {
    parent?: TestDevice;
    childId?: string;
  } = {}
): TestDevice {
  testDevice.parent = parent;
  testDevice.childId = childId;

  if (device) {
    testDevice.deviceType = device.deviceType;
    testDevice.mac = device.mac;
    testDevice.deviceOptions = {
      host: device.host,
      port: device.port,
    };
    if (childId !== undefined) {
      // @ts-ignore
      testDevice.deviceOptions.childId = childId;
    }

    testDevice.getDevice = function getDevice(
      deviceOptions?: Parameters<Client['getDevice']>[0],
      sendOptions?: Parameters<Client['getDevice']>[1]
    ): ReturnType<Client['getDevice']> {
      return client.getDevice(
        {
          ...testDevice.deviceOptions,
          defaultSendOptions: sendOptions,
          ...deviceOptions,
        } as Parameters<Client['getDevice']>[0], // TODO: I don't like this cast but only way I could get it to work
        sendOptions
      );
    };

    if ('children' in device && device.children.size > 0) {
      if (testDevice.childId === undefined) {
        testDevice.children = ((): TestDevice[] => {
          return Array.from(device.children.keys()).map((key) => {
            return createTestDevice(device, client, {
              name: testDevice.name,
              model: testDevice.model,
              hardwareVersion: testDevice.hardwareVersion,
              isSimulated: testDevice.isSimulated,
              parent: testDevice,
              childId: key,
            });
          });
        })();
      } else {
        testDevice.getOtherChildren = function getOtherChildren(): TestDevice[] {
          return parent.children.filter((oc) => oc.childId !== this.childId);
        };

        testDevice.getOtherChildrenState = async function getOtherChildrenState(): Promise<
          Array<{
            childId: string;
            relayState: boolean;
            alias: string;
          }>
        > {
          return Promise.all(
            testDevice.getOtherChildren().map(async (childDevice) => {
              const d = (await childDevice.getDevice()) as Plug;
              return {
                childId: d.childId,
                relayState: d.relayState,
                alias: d.alias,
              };
            })
          );
        };
      }
    }
  }

  testDevices.push(testDevice);
  return testDevice;
}
